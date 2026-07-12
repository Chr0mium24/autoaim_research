# 最新 RoboMaster 视觉模型仓库调研

日期：2026-07-12

## 结论摘要

这份调研覆盖当前归档中最新的 5 个视觉/自瞄模型相关仓库：

| 排名 | 仓库 | 最新 commit | 技术定位 | 主要后端 | 主要设备 |
| --- | --- | --- | --- | --- | --- |
| 1 | `Climber_Vision_26` | 2026-06-13 [`9953692`](https://github.com/CCZU-Climber/Climber_Vision_26/commit/99536929f0d0d0b9c18ed7adeb3e19650a3e9c7b) | 完整自瞄/打符框架 | OpenVINO + OpenCV DNN | x86/NUC，OpenVINO CPU/GPU |
| 2 | `AT_NN_Detector` | 2026-06-11 [`d7ceb26`](https://github.com/PraySky1337/AT_NN_Detector/commit/d7ceb26cbec2a9e1b7b65189abd8c8b914f390a5) | 装甲板关键点模型包 | ONNX Runtime + Axera AX_ENGINE | PC CUDA/CPU 预览，Axera650 部署 |
| 3 | `RobotDetectionModel` | 2026-06-02 [`babaebd`](https://github.com/broalantaps/RobotDetectionModel/commit/babaebd6c8b3aeceda0ca924772b2d70d51801b4) | OpenVINO 装甲板检测模型 | OpenVINO C++ Runtime | Intel NUC/OpenVINO GPU，CPU 可配置 |
| 4 | `jlu_vision_26` | 2026-05-29 [`0dff17c`](https://github.com/Fskaaaaaaaa/jlu_vision_26/commit/0dff17cf0e88afa4d69c6e230c3f8bb69f1cf707) | 自瞄 + 能量机关框架 | OpenVINO | x86/NUC，当前配置 CPU |
| 5 | `SHtech_auto_aim` | 2026-05-03 [`08af342`](https://github.com/Astra-Whale/SHtech_auto_aim/commit/08af342611c028a47612437df3d0cf5c9eaa57a7) | 跨硬件后端自瞄工程 | AXCL / TensorRT / MIGraphX | AX650、NVIDIA GPU、ROCm GPU |

`breeze` 的 commit 更新于 2026-06-22，但它是 Zephyr 嵌入式控制框架，不含视觉神经网络模型，所以没有放入本次模型主体比较。

总体判断：

- 如果要看完整自瞄工程闭环，优先看 `Climber_Vision_26`、`jlu_vision_26`、`SHtech_auto_aim`。
- 如果只看模型设计和部署资产，`AT_NN_Detector` 信息最集中，模型输入输出也最明确。
- 如果目标设备是 Intel NUC/OpenVINO，`Climber_Vision_26`、`RobotDetectionModel`、`jlu_vision_26` 更直接。
- 如果目标设备是 AX650/Axera，`AT_NN_Detector` 和 `SHtech_auto_aim` 更有参考价值。
- 如果要跨 CUDA/TensorRT、ROCm/MIGraphX、AXCL 做同一模型部署，`SHtech_auto_aim` 的后端抽象最值得看。

## 横向对比

### 模型任务与结构

| 仓库 | 主模型任务 | 模型结构特征 | 输出属性 |
| --- | --- | --- | --- |
| `Climber_Vision_26` | 装甲板检测、打符 pose、数字分类 | YOLOv5 / YOLOv8 / YOLO11 OpenVINO IR；打符为 YOLO11 pose；分类器为 tiny ResNet | bbox、4 角点、颜色、装甲数字/类型；打符 9 关键点 remap 成 6 点 |
| `AT_NN_Detector` | 装甲板关键点检测 | Ultralytics YOLO26n-Pose 改造；C2PSA / CoordAtt 两套；Axera 端为量化友好 raw decode | bbox、置信度、12 类装甲、4 类颜色、4 个角点 |
| `RobotDetectionModel` | 装甲板关键点检测 | 魔改 YOLOv5 + MobileNetV3 backbone | 4 角点、置信度、4 色分类、9 类目标分类 |
| `jlu_vision_26` | 装甲板检测、能量机关检测 | 装甲板为 YOLO 风格角点模型；能量机关为 YOLOX 风格关键点模型 | 装甲 4 角点、颜色、类型；能量机关 5 点、颜色、激活状态 |
| `SHtech_auto_aim` | 装甲板四角点检测 | 两套 ONNX/AXMODEL 输出形态；统一接角点修正、PnP 和跟踪 | 4 角点、置信度、tag、颜色、尺寸/装甲类型 |

### 后端与设备

| 仓库 | 后端架构 | 可运行设备 | 备注 |
| --- | --- | --- | --- |
| `Climber_Vision_26` | OpenVINO `read_model/compile_model`，YOLO wrapper；分类器实际走 OpenCV DNN | OpenVINO CPU/GPU，配置多为 `GPU`；打符模型固定 `CPU` | CMake 固定 OpenVINO 2026.1 路径；本地 XML 缺同名 `.bin`，需补齐或实测 |
| `AT_NN_Detector` | ONNX Runtime 预览；Axera AX_ENGINE + IVPS + CPU 后处理 | PC CUDA/CPU 预览；Axera650 生产部署 | README 明确 CPU 推理未测试且不作为目标；Axera 端偏工程部署 |
| `RobotDetectionModel` | OpenVINO C++ Runtime，`PrePostProcessor` 负责布局/颜色/归一化 | NUC12WSKi7 + OpenVINO GPU；源码允许传 `CPU/GPU` 等 device | 主路径没有显式 resize，调用方应保证 640x640 |
| `jlu_vision_26` | OpenVINO 推理，支持同步/异步请求；iceoryx 做进程通信 | x86/NUC OpenVINO CPU/GPU；归档配置为 `CPU` | 能量机关模型输入 480x480，装甲模型输入 640x640 |
| `SHtech_auto_aim` | 后端可编译切换：AXCL、TensorRT、MIGraphX/ROCm | AX650/Axera、NVIDIA GPU、ROCm GPU | 默认 `INFERENCE_BACKEND=AXCL`，配置指向 `.axmodel` |

## 1. Climber_Vision_26

### 仓库定位

`Climber_Vision_26` 是最新的完整视觉框架样本。它不只是模型包，还包含相机输入、传统灯条检测、YOLO 装甲检测、分类器、PnP、跟踪、打符和多相机感知。

本地详细报告：

- [架构报告](Climber_Vision_26/report.md)
- [模型报告](Climber_Vision_26/MODEL_REPORT.md)

### 模型文件

| 文件 | 类型 | 任务 | 输入 | 输出 | 属性 |
| --- | --- | --- | --- | --- | --- |
| `models/yolov5.xml` | OpenVINO IR XML | 装甲板检测 | `[1,3,640,640]` | `[1,25200,22]` | YOLOv5 风格，4 角点 + objectness + 颜色 + 数字/类型 |
| `models/yolov8.xml` | OpenVINO IR XML | 装甲板检测 | `[1,3,416,416]` | `[1,14,3549]` | Ultralytics pose，2 类 B/R，4 个 2D keypoint |
| `models/yolo11.xml` | OpenVINO IR XML | 装甲板检测 | `[1,3,640,640]` | `[1,50,8400]` | 38 类组合标签，直接输出装甲名字和四点 |
| `models/tiny_resnet.onnx` | ONNX | 装甲数字/图案分类 | 代码处理成 `32x32` 灰度 | 9 类 softmax | YOLOv8 和传统检测后的分类器 |
| `models/buff_repvgg.xml` | OpenVINO IR XML | 能量机关 pose | `[1,3,640,640]` | `[1,33,8400]` | Ultralytics pose，2 类目标，9 个关键点 |

配置中还引用了 `assets/yolo11_buff_int8.xml`，但归档目录没有复制该文件。

### 模型结构和属性

装甲检测有三套：

- `YOLOV5`：每行候选包含 8 个角点坐标、objectness、4 个颜色分数、9 个数字/名称分数。类别会映射到 `sentry/one/two/three/four/five/outpost/base/not_armor`。
- `YOLOV8`：输出 `bbox + 2 class + 4*2 keypoints`，类别只区分 B/R，具体装甲名字依赖 `tiny_resnet.onnx`。
- `YOLO11`：输出 `bbox + 38 class + 4*2 keypoints`，38 类直接编码颜色、装甲名和大小装甲属性。

打符模型 `buff_repvgg.xml` 是 pose 模型：

- 输出 `4 bbox + 2 class + 9*3 keypoints`。
- 原始 9 点会 remap 为 solver 需要的 6 点：顶、左、底、右、扇叶中心、R 中心。
- 后处理还会用传统视觉修正 R 中心、去重和筛选大符候选。

### 后端架构

主自瞄模型入口是 `code/tasks/auto_aim/yolo.cpp`，根据 YAML 的 `yolo_name` 选择 `YOLOV5 / YOLOV8 / YOLO11`。三者都走：

1. 读取 OpenVINO IR。
2. `PrePostProcessor` 声明输入为 NHWC BGR `u8`。
3. 转 NCHW RGB，除以 255。
4. `compile_model(model, device, LATENCY)`。
5. 同步推理后做 NMS、类别过滤、角点排序。

分类器 `tiny_resnet.onnx` 当前实际 `classify()` 路径走 OpenCV DNN；代码也保留了 OpenVINO `AUTO` 编译路径，但不是主链路。

### 可运行设备

- OpenVINO CPU / GPU：YAML 中 `device` 多配置为 `GPU`。
- 打符 `YOLO11_BUFF` 固定编译到 `CPU`。
- 适合 Intel NUC / x86 工控机 + OpenVINO 2026.1。

### 风险

- 归档中只有 XML，没有同名 `.bin` 权重；需要补齐权重或实测 XML 单文件能否加载。
- 配置引用 `assets/...`，归档模型放在 `models/...`，直接运行要改路径。

## 2. AT_NN_Detector

### 仓库定位

`AT_NN_Detector` 是模型分发包，不是完整自瞄框架。它集中提供装甲板检测 ONNX、Axera `.axmodel`、预览脚本和 Axera 端解码逻辑。

本地详细报告：

- [架构报告](AT_NN_Detector/report.md)
- [模型报告](AT_NN_Detector/MODEL_REPORT.md)

### 模型文件

| 文件 | 类型 | 输入 | 输出 | 设计属性 |
| --- | --- | --- | --- | --- |
| `models/common/praysky_c2psa_e2e_0228_640x640.onnx` | ONNX | fp32 `[1,3,640,640]` | fp32 `[1,30,18]` | C2PSA，End-to-End |
| `models/common/praysky_c2psa_e2e_0228_576x768.onnx` | ONNX | fp32 `[1,3,576,768]` | fp32 `[1,30,18]` | C2PSA，End-to-End |
| `models/common/praysky_coord_noe2e_0331_640x640.onnx` | ONNX | fp32 `[1,3,640,640]` | fp32 `[1,30,18]` | CoordAtt，NMS 导出 |
| `models/common/praysky_coord_noe2e_0331_576x768.onnx` | ONNX | fp32 `[1,3,576,768]` | fp32 `[1,30,18]` | CoordAtt，NMS 导出 |
| `models/axera/praysky_coord_noe2e_qat_u16_all.axmodel` | AXMODEL | 配置头缺失，宽高无法闭合确认 | raw concat-predecode | CoordAtt，QAT+PTQ，w8a16 |

### 模型结构和属性

模型基于 Ultralytics YOLO26n-Pose 改造。每条 ONNX 输出记录为 18 维：

- `0:4`：bbox，`x1,y1,x2,y2`
- `4`：confidence
- `5`：class id
- `6:10`：4 个颜色分数，`B/R/G/P`
- `10:18`：4 个角点坐标

类别数为 12，覆盖小装甲、大装甲、哨兵、前哨站、基地大装甲等映射。

C2PSA 和 CoordAtt 的差异：

- `C2PSA`：强调精度，作者用于 e2e 导出。
- `CoordAtt`：量化友好，适合 Axera 部署；将部分 anchor offset / decode 工作转移到 CPU。

### 后端架构

ONNX 预览链路：

1. `preview.py` 使用 ONNX Runtime。
2. provider 优先 `CUDAExecutionProvider`，回退 `CPUExecutionProvider`。
3. 预处理为 letterbox + `114` padding，BGR 转 RGB，归一化到 `[0,1]`。
4. 模型输出最多 30 条结果，脚本只做过滤、坐标反变换和绘制。

Axera 链路：

1. `AX_ENGINE_Init / CreateHandle / CreateContext` 加载 `.axmodel`。
2. `AX_IVPS` 优先做硬件 resize，失败回退 OpenCV resize。
3. NPU 输出 raw tensor。
4. CPU 侧做 top-k、sigmoid、颜色过滤、关键点重排、tile NMS。

### 可运行设备

- PC 离线预览：NVIDIA CUDA GPU 或 CPU ONNX Runtime。
- 生产部署：Axera650 / AX650 端侧 NPU。

注意：README 明确 CPU 推理未测试，也不是作者主要目标。

### 风险

- Axera 端缺少 `ArmorAxeraConfig` 和核心类型头，无法从归档闭合确认输入宽高、top-k、stride、阈值。
- `.axmodel` 未通过真实 Axera SDK 读取 IO metadata。

## 3. RobotDetectionModel

### 仓库定位

`RobotDetectionModel` 是深圳大学 RobotPilots 的装甲板检测模型包，核心是 ONNX 模型和 OpenVINO C++ 推理封装。

本地详细报告：

- [架构报告](RobotDetectionModel/report.md)
- [模型报告](RobotDetectionModel/MODEL_REPORT.md)

### 模型文件

| 文件 | 类型 | 输入 | 输出 | 属性 |
| --- | --- | --- | --- | --- |
| `models/Model/0526.onnx` | ONNX | `images` fp16 `[1,3,640,640]` | fp32 `[1,25200,22]` | 新版本，README 称性能/精度优于 0708 |
| `models/Model/0708.onnx` | ONNX | `images` fp32 `[1,3,640,640]` | fp32 `[1,25200,22]` | 旧版本 |

### 模型结构和属性

README 描述网络为“魔改 YOLOv5 + MobileNetV3 backbone”。每个候选 22 维：

- `0:8`：4 个关键点坐标
- `8`：置信度
- `9:13`：颜色 logits
- `13:22`：9 类数字/目标 logits

颜色：

- 红
- 蓝
- 灰
- 紫

类别：

- `G`
- `1`
- `2`
- `3`
- `4`
- `5`
- `O`
- `Bs`
- `Bb`

关键点顺序在 README 中写为左上开始逆时针，代码会重排成后处理期望的点序。

### 后端架构

OpenVINO C++ 主路径：

1. `ov::Core::read_model(model_path)` 读取 ONNX。
2. `PrePostProcessor` 声明外部输入为 `u8`、NHWC、BGR。
3. 内部转换为 `f32`、RGB，并除以 255。
4. `compile_model(model, device)` 编译到指定设备。
5. `InferRequest` 推理。
6. 后处理做 sigmoid、颜色/类别 argmax、敌方颜色过滤和 NMS。

主路径没有显式 resize，调用方应提供 640x640 级别图像。

### 可运行设备

- README 指向 NUC12WSKi7 + OpenVINO GPU。
- 源码构造函数允许传入 `device`，因此理论上可选 OpenVINO `CPU` / `GPU` 等设备。
- 归档未包含 XML/BIN，只包含 ONNX。

### 风险

- `0526.onnx` 输入是 fp16，OpenVINO 预处理声明转 f32，实际类型转换要在目标环境确认。
- 主路径不 resize，外部输入尺寸不匹配时有风险。
- 缺少样例图、运行日志和精度/速度复现实验。

## 4. jlu_vision_26

### 仓库定位

`jlu_vision_26` 是完整自瞄/能量机关框架。模型推理主要走 OpenVINO，并通过 iceoryx 组织进程间通信。

本地详细报告：

- [架构报告](jlu_vision_26/report.md)
- [模型报告](jlu_vision_26/MODEL_REPORT.md)

### 模型文件

| 文件 | 类型 | 任务 | 输入 | 输出属性 |
| --- | --- | --- | --- | --- |
| `models/0526.onnx` | ONNX | 装甲板检测 | `{1,640,640,3}` 外部 NHWC/BGR | 4 角点、置信度、颜色、9 类装甲类型 |
| `models/yolox_rune_3.6m.onnx` | ONNX | 能量机关检测 | `{1,480,480,3}` 外部 NHWC/BGR | 5 关键点、置信度、2 色、2 类激活状态 |

配置中运行路径为 `assets/...`，归档目录模型在 `models/...`。

### 模型结构和属性

装甲板 `0526.onnx`：

- YOLO 风格角点检测。
- 每行候选包含 8 个角点坐标、置信度 logit、颜色分数、装甲类型分数。
- 类别映射：`Sentry, One, Two, Three, Four, Negative, Outpost, Base, Negative`。
- 颜色映射明确看到 `Blue, Red, Extinguished`。

能量机关 `yolox_rune_3.6m.onnx`：

- YOLOX 风格网格解码。
- 输入固定 480x480。
- 输出每个候选 5 点：R 标中心和扇叶四点。
- 颜色 2 类：Red / Blue。
- 状态 2 类：Inactivated / Activated。
- stride 为 `8/16/32`。

### 后端架构

两套模型都走 OpenVINO：

1. `read_model` 加载 ONNX。
2. 装甲板模型外部输入为 `u8` NHWC/BGR，OpenVINO 预处理转 RGB、NCHW、除以 255。
3. 能量机关模型外部输入为 `f32` NHWC/BGR，letterbox 到 480x480，边界填 114，不做 `/255`。
4. 装甲模型支持单线程和多线程异步请求。
5. 能量机关模型当前代码主要是单线程 detector。

后处理方面：

- 装甲板：NMS、角点重排、PCA/矩分析修正、PnP。
- 能量机关：TopK、NMS、同类高 IoU 候选关键点加权融合、传统 R 中心修正。

### 可运行设备

- OpenVINO CPU/GPU，设备来自配置。
- 当前归档配置为 `CPU`。
- 更适合 x86 NUC / 工控机，而不是强绑定某个端侧 NPU。

### 风险

- ONNX 图输入输出名和真实 shape 未用模型解析工具验证。
- 装甲模型颜色分数代码取 4 列，但颜色映射只明确 3 个值。
- 训练数据、导出脚本、量化方式和指标未归档。

## 5. SHtech_auto_aim

### 仓库定位

`SHtech_auto_aim` 的重点是同一套自瞄检测逻辑跨不同推理后端部署。默认配置指向 AXCL / AX650，但也保留 TensorRT 和 MIGraphX/ROCm 路线。

本地详细报告：

- [架构报告](SHtech_auto_aim/report.md)
- [模型报告](SHtech_auto_aim/MODEL_REPORT.md)

### 模型文件

| 文件 | 类型 | 输入 | 输出 | 属性 |
| --- | --- | --- | --- | --- |
| `models/SKD250526.onnx` | ONNX | `image` fp32 `[1,3,512,640]` | `out` fp32 `[1,6720,21]` | 与 AXMODEL 同源的装甲角点模型 |
| `models/SKD250526.axmodel` | AXMODEL | AXCL 路径复制 HWC uint8 RGB | `out` fp32 `[1,6720,21]` | 默认配置模型，面向 AX650 |
| `models/SZU0526_fp32input_512x640_nopre_fixoutput.onnx` | ONNX | fp32 `[1,3,512,640]` | fp32 `[1,20160,22]` | ONNX/ROCm 后端备选 |

### 模型结构和属性

`SKD250526` 输出 21 维：

- `0:8`：4 个角点
- `8`：置信度
- `9:17`：8 类 tag logits
- `17:19`：2 类颜色 logits
- `19:21`：2 类装甲尺寸 logits

`SZU0526...onnx` 输出 22 维：

- `0:8`：4 个角点
- `8`：置信度 logit
- `9:13`：4 类颜色 logits
- `13:22`：9 类目标 logits

公共处理上，模型输出不是直接进入控制，而是再经过：

1. overlap / NMS 去重。
2. 角点顺序调整。
3. 传统视觉角点 refine。
4. PnP 位姿恢复。
5. Tracker 和 TinyMPC / planner。

### 后端架构

工程通过 `INFERENCE_BACKEND` 选择后端：

- `AXCL`：默认路径，加载 `.axmodel`，面向 AX650/Axera。
- `TRT`：TensorRT / CUDA，解析 ONNX 后在网络内做 TopK，最多保留 128 个候选。
- `ONNX`：实际是 MIGraphX / ROCm 路线，加载 ONNX 或 cache。

公共预处理：

- 原图直接 resize 到 `640x512`。
- BGR 转 RGB。
- ONNX/TensorRT 走 NCHW fp32 归一化。
- AXCL 路径直接复制 HWC uint8 RGB。

### 可运行设备

- AX650 / Axera：默认目标，使用 `.axmodel` + AXCL。
- NVIDIA GPU：TensorRT 路线。
- AMD/ROCm GPU：MIGraphX 路线。

这是五个仓库里跨硬件部署抽象最明确的一个。

### 风险

- 缺少标签表和模型 README，`SKD250526` 的 8 类 tag 精确语义只能从下游间接推断。
- TensorRT 与 AXCL 对点坐标解码路径不完全一致，需要同图验证。
- `SZU0526...onnx` 未被默认配置引用，实际生产是否使用需要核对构建参数。

## 设备选型建议

### Intel NUC / x86 + OpenVINO

优先级：

1. `Climber_Vision_26`
2. `RobotDetectionModel`
3. `jlu_vision_26`

原因：

- 三者都已有 OpenVINO 路线。
- `Climber_Vision_26` 和 `jlu_vision_26` 是完整框架，便于看闭环。
- `RobotDetectionModel` 更像单模型推理包，适合抽模型和后处理。

注意：

- `Climber_Vision_26` 的 IR XML 归档缺 `.bin`。
- `RobotDetectionModel` 需要确认输入尺寸和 fp16/fp32 转换。

### AX650 / Axera

优先级：

1. `SHtech_auto_aim`
2. `AT_NN_Detector`

原因：

- `SHtech_auto_aim` 有 AXCL 工程集成和默认 `.axmodel` 配置。
- `AT_NN_Detector` 有明确的 Axera raw decode、IVPS resize 和 CPU 后处理实现。

注意：

- `AT_NN_Detector` 缺少部分配置头，不能直接完整编译。
- 两者都需要目标板 SDK 和实机验证。

### NVIDIA GPU / TensorRT

优先看 `SHtech_auto_aim`。

它的 TensorRT 后端有独立模块，并且做了 TopK 输出裁剪。其他几个仓库虽然可能能通过 OpenVINO GPU 或 ONNX 转换跑在 NVIDIA 环境，但归档里没有同等完整的 TensorRT 路线。

### 只想研究模型结构

优先级：

1. `AT_NN_Detector`
2. `RobotDetectionModel`
3. `Climber_Vision_26`

原因：

- `AT_NN_Detector` 的模型文件、README、输出语义最集中。
- `RobotDetectionModel` 模型简单清晰，输出维度固定。
- `Climber_Vision_26` 模型种类最多，但工程耦合更重。

## 可复用点

- 角点模型优于纯 bbox：五个仓库几乎都把装甲板检测设计成“框 + 角点 + 颜色/类别”，后端直接接 PnP。
- OpenVINO 是 2026 视觉框架的主流路线之一：`Climber_Vision_26`、`RobotDetectionModel`、`jlu_vision_26` 都以 OpenVINO 为核心。
- AX650 路线通常需要把后处理拆回 CPU：`AT_NN_Detector` 和 `SHtech_auto_aim` 都有“端侧 NPU + CPU decode/NMS/几何”的结构。
- 传统视觉仍然重要：`Climber_Vision_26`、`jlu_vision_26`、`SHtech_auto_aim` 都不是纯网络输出直用，而是用传统几何修角点、过滤异常或修 R 中心。
- 模型输入尺寸集中在 `640x640`、`416x416`、`480x480`、`512x640`、`576x768`，迁移时要先统一相机分辨率、letterbox/resize 策略和坐标反变换。

## 已知缺口

- 多数仓库缺训练配置、数据集说明、导出脚本和完整精度指标。
- 部分 OpenVINO IR 只有 XML，没有 `.bin`，运行前必须补齐或验证 XML 是否内嵌权重。
- 部分模型只从代码推断输出布局，未用 ONNX/OpenVINO 工具逐一解析并保存结构表。
- 多后端工程需要同图对齐验证，尤其是 `SHtech_auto_aim` 的 AXCL 与 TensorRT 路线。
- 本文是静态源码和模型文件调研，不等价于实机性能测试。
