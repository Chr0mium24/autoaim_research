# AT_NN_Detector 架构报告

## 项目定位

这是一个面向 RoboMaster 装甲板检测的模型包，不是完整自瞄框架。仓库同时提供：

1. 4 个 ONNX 推理模型，覆盖 `C2PSA` / `CoordAtt` 两种注意力方案，以及 `640x640` / `576x768` 两种输入分辨率。
2. 1 个面向 Axera650 的 `.axmodel` 部署模型。
3. 预览与推理示例代码，以及 Axera 端的推理封装。

整体定位是“模型 + 推理后端 + 预览脚本”的轻量分发包，便于直接跑图、做部署适配和迁移到其它自瞄工程。

## 模型架构

仓库 README 说明模型基于 Ultralytics 系列 YOLO26n-Pose 改造，任务输出包含：

- 检测框 `x1, y1, x2, y2`
- 目标类别，12 类
- 颜色属性，4 类：`B / R / G / P`
- 4 个关键点，按装甲板四角顺序输出

从导出命名和说明看，当前有两类导出形态：

- `praysky_c2psa_e2e_*`：P5 使用 `C2PSA`，端到端导出，模型内部带 TopK / 后处理。
- `praysky_coord_noe2e_*`：P5 使用 `CoordAtt`，非端到端导出，模型内部带 NMS，量化友好。

Axera 端的 `.axmodel` 不是最终 NMS 后结果，而是 concat-predecode 形式：`4 + num_colors + num_pairs + num_kpts*2` 的原始输出，后处理在 CPU 侧完成。

## 输入输出

### ONNX 预览链路

`preview.py` 中的 ONNX 路径采用：

- 输入：`RGB`，`float32`，范围 `[0, 1]`
- 预处理：`letterbox + pad(114)`，再做 `BGR -> RGB`
- 输出：按 README / 预览脚本的统一解析方式读取为 `1 x 30 x 18`

其中每条检测记录的语义为：

- `0:4`：框
- `4`：置信度
- `5`：类别 ID
- `6:10`：颜色分数
- `10:18`：4 个关键点

### Axera 端链路

`axera/axera.cpp` 的真实部署路径更接近硬件工程：

- 输入：`BGR888`
- 预处理：优先 IVPS 硬件 resize，失败则回退 OpenCV `cv::resize`
- 模型输出：anchor 级 concat-predecode tensor
- 后处理：CPU 侧解码颜色、类别、关键点，再做 tile-based NMS

这条链路还支持直接 resize 到网络输入，不强制 letterbox；代码里也明确提醒非 4:3 输入会走直缩放路径。

## 推理后端

仓库里实际出现了三种推理后端风格：

1. `onnxruntime`：用于 `preview.py`，并优先尝试 `CUDAExecutionProvider`，再落到 `CPUExecutionProvider`。
2. Axera NPU：`axera/axera.cpp` 通过 `AX_ENGINE_*` 和 `AX_SYS_*` API 完成模型加载、输入输出缓冲区管理和同步推理。
3. IVPS：Axera 端用于硬件加速 resize，减少 CPU 预处理开销。

## 后处理 / 跟踪预测

仓库没有跟踪器或运动预测模块，所有时序逻辑都在外部系统完成。

后处理分两套：

- ONNX 预览链路：按模型已经输出好的检测结果直接绘图，不再做额外 NMS。
- Axera 链路：先从 concat-predecode tensor 中取出 `color logits`、`obj logits`、`kpts`，做 top-k 过滤、颜色筛选、关键点重排和 tile NMS。

`axera.cpp` 里的 `PAIR_TO_ARMOR` 和 `KPT_ORDER` 是最关键的解码映射。

## 关键配置

仓库没有独立的 `yaml/json/toml` 配置文件，模型参数主要写在 README 和代码常量里。输出目录中我保留了：

- `models/README.md`：原始模型说明、类别定义和输入输出约定
- `code/preview.py`：ONNX 预览时的模型列表、阈值和绘图逻辑
- `code/axera/axera.hpp` / `axera.cpp`：Axera 端输入尺寸、stride、top-k、NMS、颜色过滤和硬件预处理逻辑

## 关键代码

- `code/preview.py`：4 个 ONNX 模型统一预览、letterbox 预处理、输出解码与可视化。
- `code/axera/axera.hpp`：Axera 后端接口、缓存结构、NMS 缓冲区和预处理上下文。
- `code/axera/axera.cpp`：模型加载、IVPS 初始化、输入写入、concat-predecode 解码、tile NMS。

## 可复用建议

- 适合直接复用的部分：
  - 类别定义和颜色分支设计
  - `576x768` 与 `640x640` 双输入分辨率导出思路
  - Axera 端的硬件 resize + CPU 后处理分层方式
  - 关键点顺序重排和 tile NMS 的实现
- 需要适配的部分：
  - `PAIR_TO_ARMOR` 与比赛年份强相关，别直接照搬到新赛季
  - ONNX 预览脚本的 letterbox 逻辑适合离线验证，不一定适合实时链路
  - Axera 端的 IVPS / NPU API 只能在对应平台上用
- 仅作参考的部分：
  - README 中的训练指标和部署性能数字，只能当作者环境下的参考值

## 备注

`preview.py` 里保留了一段关于输出维度的旧注释，但实际代码与 README 的统一解析以当前实现为准。这里的报告按源码现状整理，不按历史注释推断。

