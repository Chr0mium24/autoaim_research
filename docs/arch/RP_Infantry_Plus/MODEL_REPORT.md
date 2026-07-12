# RP_Infantry_Plus 模型报告

## 1. 模型清单

本项目是传统 CV 主导、轻量 DNN 可选辅助的步兵视觉方案。`models/README.md` 明确说明“大体积权重未复制，仅保留模型定义和配置”。

已复制的模型定义文件：

- [models/armor_deploy.prototxt](models/armor_deploy.prototxt)：Caffe LeNet，二分类输出。
- [models/armor_lenet_train_test_deploy.prototxt](models/armor_lenet_train_test_deploy.prototxt)：Caffe LeNet，8 类输出。
- [models/rune_lenet_deploy.prototxt](models/rune_lenet_deploy.prototxt)：Caffe LeNet，二分类输出。
- [models/rune_tiny_yolov2_noBatch.cfg](models/rune_tiny_yolov2_noBatch.cfg)：Darknet tiny-YOLOv2 配置。

仅被 README/代码引用、但本地未复制的权重：

- 装甲板 Caffe：`lenet_iter_200000.caffemodel`、`armornet_iter_200000.caffemodel`。
- 大符 LeNet：`lenet_iter_80000*.caffemodel`。
- 大符 YOLO：`tiny-yolov2-trial3-noBatch_*.weights`。

## 2. 单模型细节

### 装甲板 LeNet

[ArmorDetector.hpp](code/include/Aim/ArmorDetector.hpp) 中硬编码 `readNetFromCaffe(...lenet_train_test_deploy.prototxt, ...lenet_iter_200000.caffemodel)`。复制出来的 [armor_lenet_train_test_deploy.prototxt](models/armor_lenet_train_test_deploy.prototxt) 显示输入为 `1x1x28x28` 灰度图，结构为 conv20、pool、conv50、pool、fc500、fc8、Softmax。

任务是装甲板候选分类，代码注释称用于哨兵场景的多分类/二分类辅助；8 类的具体类别名本地没有给出。另一个 [armor_deploy.prototxt](models/armor_deploy.prototxt) 是 `fc2 + Sigmoid`，但未在本地代码中看到直接路径引用。

### 大符 LeNet

[Detect.cpp](code/src/Rune/Detect.cpp) 构造函数用 `readNetFromCaffe(lenet_txt_file, lenet_model_file)` 加载，路径来自 [header.h](code/include/header.h)。复制的 [rune_lenet_deploy.prototxt](models/rune_lenet_deploy.prototxt) 输入为 `1x1x28x28`，输出为 `fc2 + Softmax`。

任务是区分候选扇叶是否为有效目标，代码注释写明 `classId == 0` 为 `noise`，`classId == 1` 为 `true`。默认 `sParam.use_lenet = 0`，因此是可选链路。

### 大符 tiny-YOLOv2

[rune_tiny_yolov2_noBatch.cfg](models/rune_tiny_yolov2_noBatch.cfg) 定义 `448x448x1` 输入、5 个 anchor、`classes=1`、`coords=4`、region 输出。代码通过 `readNetFromDarknet(yolo_txt_file, yolo_model_file)` 加载，默认 `sParam.use_yolo = 0`。

[Detect::forward](code/src/Rune/Detect.cpp) 先转灰度，再用 `blobFromImage(gray, 0.00390625, Size(448,448))`，代码按每行 `x/y/w/h + class probability` 取最大置信度，阈值为 `0.8`，再把检测框放大为 ROI。OpenCV DNN 的精确输出矩阵形状未由本地文件证明。

## 3. 预处理、后处理与传统视觉

装甲板主链路不是端到端检测网络。[ArmorDetector.cpp](code/src/Aim/ArmorDetector.cpp) 使用上一帧 ROI、灰度阈值、红蓝通道差、膨胀、轮廓、`minAreaRect`、灯条几何约束和候选排序；之后 [ImageConsProd.cpp](code/src/ImageConsProd.cpp) 调用 `AngleSolver` 做 PnP，距离还用了灯条高度经验拟合。

大符链路在 [Detect.h](code/include/Rune/Detect.h) 和 [Detect.cpp](code/src/Rune/Detect.cpp)。默认使用上一帧 ROI 和 GRAY 二值化，可切换 BGR/HSV/YCrCb/LUV/OTSU 等模式；通过轮廓层级、面积、长宽比、矩匹配筛选扇叶和装甲中心。大符预测支持 `FIT_CIRCLE`、`PUSH_CIRCLE`、`TANGENT`，默认 `TANGENT`。

串口输出由 [serialport.cpp](code/src/SerialPort/serialport.cpp) 打包 22 字节帧，字段包括 pitch、yaw、distance 和状态位；README 中的 30 字节协议与代码实现不完全一致，应以代码为准。

## 4. 部署后端

推理后端是 OpenCV DNN：

- Caffe：`cv::dnn::readNetFromCaffe`
- Darknet：`cv::dnn::readNetFromDarknet`

未看到 ONNX、TensorRT、OpenVINO、CUDA/TensorRT engine 或 DeepStream 部署入口。README 描述运行环境为 Ubuntu 16.04、OpenCV 3.4.4、Qt Creator、CMake/GCC，硬件为 Dahua 工业相机、TTL-USB 串口和 Intel NUC。

## 5. 未知与验证缺口

- 权重文件未复制，无法验证模型能否加载、实际类别语义和精度。
- `header.h` 和 `ArmorDetector.hpp` 保留原始绝对路径，当前归档目录不能直接运行。
- 装甲板 8 类 LeNet 的类别映射、本地训练集和标签定义缺失。
- YOLO 的 OpenCV DNN 输出布局只由解析代码间接推断，未见导出说明。
- 默认 `use_yolo/use_lenet` 为 0，但构造函数仍会加载网络；缺权重时实际运行行为需要实机验证。
