# RobotDetectionModel 模型报告

## 模型清单

- [models/Model/0526.onnx](models/Model/0526.onnx)：README 称为较新的 0526 版本。
- [models/Model/0708.onnx](models/Model/0708.onnx)：较旧版本。
- [models/README.md](models/README.md)：模型任务、类别、环境说明。
- [code/OpenvinoInfer.cpp](code/OpenvinoInfer.cpp)、[code/OpenvinoInfer.h](code/OpenvinoInfer.h)：OpenVINO C++ 推理封装。
- 未归档：训练权重、训练/导出脚本、样例图、OpenVINO IR `.xml/.bin` 文件。

## 模型细节

| 文件 | 输入 | 输出 | 备注 |
| --- | --- | --- | --- |
| `0526.onnx` | `images` fp16 `[1,3,640,640]` | `output` fp32 `[1,25200,22]` | 新版本，README 称精度和性能优于 0708 |
| `0708.onnx` | `images` fp32 `[1,3,640,640]` | `output` fp32 `[1,25200,22]` | 旧版本 |

- 任务：RoboMaster 装甲板检测。
- 网络：README 描述为“魔改 YOLOv5 + MobileNetV3 backbone”；本地 ONNX 未进一步证明具体层级改动。
- 每个候选输出 22 维：[0:8] 为 4 个关键点，[8] 为置信度，[9:13] 为颜色 logits，[13:22] 为数字/目标类别 logits。
- 关键点：README 写明从左上角开始逆时针；[code/OpenvinoInfer.cpp](code/OpenvinoInfer.cpp) 会重排为左上开始的顺时针点序用于矩形/NMS。
- 颜色：红、蓝、灰、紫；源码会丢弃灰/紫，仅保留指定敌方颜色。
- 类别：9 类，README 定义为 `G, 1, 2, 3, 4, 5, O, Bs, Bb`。

## 预处理与后处理

- 主路径预处理：[code/OpenvinoInfer.cpp](code/OpenvinoInfer.cpp) 使用 OpenVINO `PrePostProcessor`，外部 tensor 为 `u8`、`NHWC`、BGR，内部转换为 `f32`、RGB、除以 `255`，模型布局设为 `NCHW`。
- 输入尺寸：ONNX 元数据为 `640x640`；`.cpp` 主路径没有显式 resize，调用方需要提供匹配尺寸的图像。
- 旧路径：[code/OpenvinoInfer.h](code/OpenvinoInfer.h) 中还有一个灰度 `1x1x640x640` 重载，会 resize 并转灰度，只复制 9 个输出值；它与当前 ONNX 输出不匹配，不能视为主部署路径。
- 后处理：置信度 sigmoid 后按 `0.65` 过滤；颜色/类别取 argmax；根据 `detect_color` 过滤红/蓝；由 4 点外接矩形做 `cv::dnn::NMSBoxes`，NMS 阈值 `0.45`。

## 部署后端

- 后端：OpenVINO C++ Runtime，使用 `ov::Core`、`ov::CompiledModel`、`ov::InferRequest`。
- 目标设备：README 指向 NUC12WSKi7 + OpenVINO GPU；源码构造函数通过 `device` 参数选择设备。
- 模型格式：归档提供 ONNX；源码也保留 XML/BIN 构造函数，但本目录没有 IR 文件。

## 未知与核验缺口

- 未提供验证图和运行日志，未核验 0526/0708 的实际精度与速度。
- `0526.onnx` 输入是 fp16，而主预处理声明转换到 f32；OpenVINO 是否自动插入转换需要在目标环境确认。
- `.cpp` 主路径没有 resize，若调用方传入非 `640x640` 图像可能存在输入内存/形状风险。
- NMS 的 `scores_nms` 使用的是数字类别最大分 `score_num`，不是 sigmoid 后的目标置信度；是否为有意设计需要实测确认。
