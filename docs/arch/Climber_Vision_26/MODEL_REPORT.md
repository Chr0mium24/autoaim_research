# Climber_Vision_26 模型报告

## 模型清单

| 文件 | 本地状态 | 代码/配置引用 | 备注 |
| --- | --- | --- | --- |
| [models/yolov5.xml](models/yolov5.xml) | 已复制 XML | `configs/*.yaml` 引用 `assets/yolov5.xml` | 主自瞄 YOLOV5；本地无 `assets/` 副本，需改路径或复制资产。 |
| [models/yolov8.xml](models/yolov8.xml) | 已复制 XML | `configs/*.yaml` 引用 `assets/yolov8.xml` | 主自瞄 YOLOV8；`sentry.yaml` 选择该模型。 |
| [models/yolo11.xml](models/yolo11.xml) | 已复制 XML | `configs/*.yaml` 引用 `assets/yolo11.xml` | 主自瞄 YOLO11；XML 无 Ultralytics 元数据。 |
| [models/tiny_resnet.onnx](models/tiny_resnet.onnx) | 已复制 ONNX | `classify_model: assets/tiny_resnet.onnx` | 装甲数字分类器；本地路径同样与配置不一致。 |
| [models/buff_repvgg.xml](models/buff_repvgg.xml) | 已复制 XML | `infantry_4/5.yaml`、`test_buff.yaml` 引用 `assets/buff_repvgg.xml` | 打符 pose 模型；元数据写明 `buff-pose-repvgg`。 |
| `assets/yolo11_buff_int8.xml` | 未复制 | [configs/uav.yaml](configs/uav.yaml) 引用 | 打符配置缺口；本目录只有 `buff_repvgg.xml`。 |

未发现 `.bin` 文件。上述 IR XML 内有大量 `data offset/size` 常量引用，按 OpenVINO IR 习惯通常需要同名 `.bin` 权重；未实际运行 `read_model()`，不能确认 XML 单文件可加载。

## 主自瞄模型

入口是 [code/tasks/auto_aim/yolo.cpp](code/tasks/auto_aim/yolo.cpp)，按 `yolo_name` 选择 `yolov5`、`yolov8`、`yolo11`。三者都用 OpenVINO `read_model/compile_model`，设备来自 `device`，同步推理为 `LATENCY`。

### yolov5.xml

- 任务：装甲板检测、颜色/数字识别、4 点输出。
- 格式：OpenVINO IR XML；XML 输入 `[1,3,640,640]`，输出 `[1,25200,22]`。
- 预处理：代码创建 `640x640` 黑底图，按比例缩放贴左上角；`PrePostProcessor` 声明输入 NHWC `u8` BGR，转 NCHW RGB，除以 255。
- 输出解析：[yolov5.cpp](code/tasks/auto_aim/yolos/yolov5.cpp) 将每行解释为 8 个角点坐标、1 个 objectness、4 个颜色分数、9 个数字/名称分数；objectness 过 sigmoid。
- 后处理：`score_threshold_ = 0.7`、NMS `0.3`、`min_confidence` 过滤；`use_traditional` 为真时调用传统灯条法二次修角点。
- 类别语义：颜色由 `color_id` 映射到 blue/red/extinguish；数字分支映射到 `sentry/one..five/outpost/base/not_armor`。

### yolov8.xml

- 任务：装甲颜色/灯条 pose 检测，装甲名称依赖分类器。
- 格式：OpenVINO IR XML；XML 输入 `[1,3,416,416]`，输出 `[1,14,3549]`。
- 元数据：Ultralytics pose，`imgsz [416,416]`，`names {0:'B',1:'R'}`，`kpt_shape [4,2]`。
- 预处理：`416x416` letterbox，NHWC BGR `u8` -> NCHW RGB `f32/255`。
- 输出解析：转置为候选行；`4 bbox + 2 class + 4*2 keypoints`。4 点排序为左上、右上、右下、左下。
- 后处理：先按 B/R 类别和 NMS 取候选，再裁剪图案送 `tiny_resnet` 分类名称；本项目 YOLOV8 仍会在 `use_traditional` 为真时调用传统修正。

### yolo11.xml

- 任务：装甲板检测，直接输出组合类别和 4 点。
- 格式：OpenVINO IR XML；XML 输入 `[1,3,640,640]`，输出 `[1,50,8400]`。
- 输出解析：`4 bbox + 38 class + 4*2 keypoints`；38 类由 [armor.hpp](code/tasks/auto_aim/armor.hpp) 的 `armor_properties` 映射到颜色、名称和大小装甲。
- 后处理：转置、取最大类分数、NMS、4 点排序、名称/大小合法性过滤。
- 差异点：本项目 `YOLO11` 中传统 `Detector` 成员被注释，配置里的 `use_traditional` 不会作用到 YOLO11。

## 分类器

[models/tiny_resnet.onnx](models/tiny_resnet.onnx) 用于传统检测和 YOLOV8 的图案分类。[classifier.cpp](code/tasks/auto_aim/classifier.cpp) 先用 OpenCV DNN 读取 ONNX，也编译了 OpenVINO `AUTO` 模型；实际 `classify()` 走 OpenCV DNN，`ovclassify()` 未在检测链路中看到调用。

输入处理：裁剪图案转灰度，按比例塞入 `32x32` 黑底，`blobFromImage(..., 1/255)`。输出按代码固定为 9 类 softmax，并映射到 `ArmorName`。

## 打符模型

[models/buff_repvgg.xml](models/buff_repvgg.xml) 是 Climber 当前复制到位的打符模型。

- 任务：能量机关目标 pose 检测。
- 格式：OpenVINO IR XML；XML 输入 `[1,3,640,640]`，输出 `[1,33,8400]`。
- 元数据：Ultralytics `task=pose`，`kpt_shape [9,3]`，类别 `{0:'r_target',1:'b_target'}`。
- 后端：`YOLO11_BUFF` 固定 `compile_model(model, "CPU")`。
- 预处理：默认 `fill_tensor_data_image()`，按比例 `warpAffine` 到 `640x640`，BGR->RGB，归一化到 `[0,1]`，写入 NCHW `f32` tensor；旧预处理分支存在但默认关闭。
- 输出解析：`4 bbox + 2 class + 9*3 keypoints`；根据 `enemy_color` 反选要打的己方颜色类别。
- 关键点 remap：原始 9 点中 `raw[0]` 是 R 中心，`[1,8]` 底、`[2,3]` 右、`[4,5]` 顶、`[6,7]` 左；代码 remap 成旧 solver 需要的 6 点：顶、左、底、右、扇叶中心、R 中心。
- 传统补偿：Buff_Detector 用二值化、膨胀、R 中心 mask 细化、去重、小符连续性约束和大符候选筛选，不是纯网络输出直用。

## 部署后端

- CMake 固定 OpenVINO 路径 `/opt/intel/openvino_2026.1.0/runtime/cmake/`。
- 主自瞄 YOLO 使用 YAML 的 `device`，配置多为 `GPU`；打符固定 `CPU`；分类器 `classify()` 实际用 OpenCV DNN。
- `configs/` 使用 `assets/...` 路径，但本报告目录只复制了 `models/...`，直接从本目录运行会缺模型路径。

## 未知与验证缺口

- 未复制任何 `.bin`，且 XML 中存在权重 offset；需要补齐权重或实测 `read_model(xml)`。
- `assets/yolo11_buff_int8.xml` 被配置引用但未复制。
- `tiny_resnet.onnx` 未用 ONNX 工具解析，只能从代码确认 `32x32` 灰度输入和 9 类输出。
- 未运行实机/离线推理，无法确认 OpenVINO 版本、GPU 插件和模型文件是否匹配。
