# robomaster-cv 模型报告

## 模型清单

- Darknet YOLOv3：配置 [configs/param-yolo.yaml](configs/param-yolo.yaml) 引用 `../detection/data/dji.data`、`yolov3_custom.cfg`、`yolov3_custom_best.weights`、`dji.names`，这些文件未归档。
- Darknet YOLOv3 Tiny：配置 [configs/param-yolo-tiny.yaml](configs/param-yolo-tiny.yaml) 引用 `yolov3_tiny_custom.cfg`、`yolov3_tiny_custom_best.weights`、`dji.names`，这些文件未归档。
- DeepStream YOLO：配置 [configs/deepstream-infer.txt](configs/deepstream-infer.txt) 引用绝对路径 `/home/polystar/robomaster-2024-cv/detection/data/yolov5s.cfg`、`yolov5s.wts`、`dji.names` 和生成的 `model_b1_gpu0_fp32.engine`，均未归档。
- 已归档 [models/icra.xml](models/icra.xml)、[models/std.xml](models/std.xml)：机器人 URDF 简化模型；[models/icra.yaml](models/icra.yaml)、[models/std.yaml](models/std.yaml)：轮径、底盘尺寸、编码器参数。它们不是视觉神经网络权重。

## Darknet YOLOv3 / YOLOv3 Tiny

- 任务：从相机图像检测目标框、类别和置信度，发布 `Detections`。
- 格式/后端：Darknet `.cfg + .weights + .data + .names`，由 [ros_ws/src/detection/src/detector.cpp](ros_ws/src/detection/src/detector.cpp) 调用 `parse_network_cfg_custom` 和 `load_weights`。
- 输入：ROS 图像转 OpenCV Mat 后转换为 Darknet `image`，像素除以 255，再 resize 到网络配置里的 `net.w/net.h`。由于 `.cfg` 缺失，确切输入尺寸未知。
- 输出：`get_network_boxes` 返回 bbox 和各类概率；代码按阈值 `tresh=0.24` 选择最高类别，输出 `x,y,w,h,clss,score`。`nms=0.45` 在配置中存在，但当前源码未看到显式 NMS 调用。
- 类别：依赖缺失的 `dji.names`。决策代码暗示存在装甲颜色类和 `Base/Standard/Hero/Sentry` 等容器类，但准确类别顺序不能确认。

## DeepStream YOLO / TensorRT

- 任务：Jetson 上通过 DeepStream 管线执行 YOLO 检测，并把 `NvDsObjectMeta` 转成检测消息。
- 格式/后端：DeepStream-Yolo 自定义解析库 + `yolov5s.cfg` + `yolov5s.wts`，运行时生成/使用 TensorRT engine。
- 输入：DeepStream app 使用 V4L2 相机源 `640x480@30`；`streammux` 输出 `480x480`、不 padding；infer 配置 `net-scale-factor=1/255`、RGB、batch 1、FP32。
- 输出：`num-detected-classes=6`，`NvDsObjectMeta` 中的 `class_id/confidence/left/top/width/height` 被映射为检测框。DeepStream 配置的 `pre-cluster-threshold=0.25`、`nms-iou-threshold=0.6`、`cluster-mode=2` 控制后处理。
- 类别：仍依赖缺失的 `dji.names`；DeepStream 配置的 6 类与决策代码里 `Sentry=6` 的枚举存在潜在不一致，需实机核对。

## 机器人模型：icra/std

- 任务：用于定位/TF 和底盘参数，不参与图像推理。
- 格式：[models/icra.xml](models/icra.xml)、[models/std.xml](models/std.xml) 是 URDF；YAML 保存 `wheel_radius, l_x, l_y, encoder_resolution`。
- 部署：启动文件把 ICRA 模型传给 locate/mock locate，供 `robot_description` 和运动学参数使用。

## 预处理/后处理总览

- Darknet 路径：图像转 Darknet CHW float，resize 到网络尺寸；后处理取 Darknet bbox、类别最大概率和阈值过滤。
- DeepStream 路径：GStreamer/DeepStream 负责缩放、归一化、YOLO 解析、聚类/NMS；项目代码只把 DeepStream metadata 转 ROS 消息。
- 检测后还有 [ros_ws/src/tracking/scripts/track.py](ros_ws/src/tracking/scripts/track.py) 的 motpy 多目标跟踪，以及 decision 节点的启发式取靶；这些不是神经网络模型。

## 部署后端

- 本机/训练验证路径：Darknet。
- Jetson 路径：NVIDIA DeepStream + TensorRT engine。主启动文件当前传参倾向 DeepStream `true`、Darknet `false`。
- 项目文档同时出现 ROS2 Iron/ament 和旧 Jetson 文档中的 ROS Melodic/catkin 描述，部署年代信息不统一。

## 未知与验证缺口

- 所有检测权重、网络 cfg、label names 和 data 文件都缺失，无法确认准确类别、输入尺寸、anchor、网络结构和精度。
- Darknet 配置引用 YOLOv3，DeepStream 配置引用 YOLOv5s，实际生产使用哪一套需核对。
- DeepStream 代码存在 ROS 消息命名和构建细节混杂，未做编译验证。
- 类别编号与 decision 代码之间存在潜在错位，尤其 `num-detected-classes=6` 与 `Sentry=6`。
