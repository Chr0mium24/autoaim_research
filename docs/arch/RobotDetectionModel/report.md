# RobotDetectionModel 架构报告

## 项目定位

这是深圳大学 RobotPilots 的装甲板检测模型包，更偏向“单模型 + 单推理封装”的发布形式。仓库核心内容只有：

1. 两个 ONNX 模型：`0526.onnx` 和 `0708.onnx`
2. 一个 OpenVINO C++ 推理封装：`OpenvinoInfer.cpp/.h`
3. README 中的类别说明、环境说明和部署提示

它不是完整自瞄系统，没有相机驱动、串口通信、目标跟踪或弹道预测模块。

## 模型架构

README 描述网络为“魔改 YOLOv5 + MobileNetV3 backbone”，目标是装甲板检测。输出由四部分组成：

- 4 个关键点
- 置信度
- 颜色分类：红 / 蓝 / 灰 / 紫
- 数字或目标类别：`G, 1, 2, 3, 4, 5, O, Bs, Bb`

从 `OpenvinoInfer.cpp` 的后处理可以反推出输出张量的一条检测记录格式大致为：

- `0:8`：8 个关键点坐标
- `8`：置信度
- `9:13`：颜色 logits
- `13:22`：9 类数字/目标 logits

这意味着模型是“关键点 + 多头分类”的装甲板检测器，而不是纯框检测器。

## 输入输出

### 输入

主实现 `OpenvinoInfer.cpp` 使用：

- 输入尺寸：`640 x 640`
- 输入类型：`u8`
- 输入布局：`NHWC`
- 颜色：`BGR -> RGB`
- 归一化：除以 `255`

主链路里没有显式 `resize`，而是把输入张量直接交给 OpenVINO 的预处理图处理。也就是说，外部传入的 `img` 需要已经是 `640x640` 级别的图像，颜色和归一化则由 `PrePostProcessor` 负责。

### 输出

输出张量在后处理中被视为二维表，每行一条候选：

- 置信度先过 sigmoid
- 颜色和数字类别都走 `argmax`
- 再根据 `detect_color` 做目标颜色过滤
- 最后把 4 个角点拼成矩形框，交给 `cv::dnn::NMSBoxes`

## 推理后端

仓库使用 OpenVINO：

- `ov::Core`
- `ov::preprocess::PrePostProcessor`
- `ov::CompiledModel`
- `ov::InferRequest`

README 明确面向 NUC12WSKi7 + OpenVINO GPU，强调 OpenVINO 24 及 Intel GPU 驱动环境。

代码里还保留了一个基于 `model_path` 的重载构造与一个 `float* detections` 的轻量推理接口，但主实现路径是 `OpenvinoInfer.cpp` 中的 XML/BIN 或 ONNX 编译执行链路。

## 后处理 / 跟踪预测

仓库没有跟踪器和预测器，只有单帧后处理：

- 置信度阈值过滤
- 颜色过滤：只保留蓝或红，灰色和紫色直接丢弃
- 按检测框做 NMS

因此它适合被嵌入到外部自瞄框架里做检测层，不适合单独承担完整目标管理逻辑。

## 关键配置

仓库同样没有独立配置文件，重要参数都在源码和 README 里：

- `IMAGE_HEIGHT = 640`
- `IMAGE_WIDTH = 640`
- `conf_threshold = 0.65`
- `nms_threshold = 0.45`
- `detect_color` 参数控制蓝/红目标过滤

输出目录里我保留了：

- `models/README.md`：原始模型说明与类别定义
- `code/OpenvinoInfer.cpp`
- `code/OpenvinoInfer.h`

## 关键代码

- `code/OpenvinoInfer.cpp`：OpenVINO 模型加载、预处理图构建、推理和后处理主逻辑。
- `code/OpenvinoInfer.h`：`Object` 结构、推理类接口，以及一个旧式轻量推理重载。

## 可复用建议

- 适合直接复用的部分：
  - 关键点 + 颜色 + 类别的多头输出设计
  - OpenVINO PrePostProcessor 的 BGR->RGB + scale 预处理方式
  - 颜色过滤和单类目标筛选逻辑
  - 用关键点构造目标矩形再做 NMS 的后处理框架
- 需要适配的部分：
  - 只支持固定 640x640，换相机或换分辨率时要改预处理
  - 类别集合明显绑定该队伍的赛事目标定义，不能原样迁移
  - 如果外部系统需要更稳的多目标管理，还要补 tracker
- 仅作参考的部分：
  - README 的 OpenVINO 安装步骤和设备性能描述，依赖具体 NUC/GPU 环境

## 备注

`OpenvinoInfer.h` 里存在一条偏旧的灰度输入路径，但当前更完整的实现是 `OpenvinoInfer.cpp` 中的彩色输入流程。整理时以 `.cpp` 的主流程为准。
