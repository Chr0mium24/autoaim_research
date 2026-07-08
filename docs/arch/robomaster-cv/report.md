# robomaster-cv 架构报告

## 项目定位

这是 PolySTAR 2026 RoboMaster 视觉组的 ROS2 视觉框架，核心职责是把相机图像、识别结果、跟踪结果和下位机控制串成一条闭环。项目既支持 Darknet 训练/推理，也支持 Jetson 上的 DeepStream 部署。

项目主线不是单一检测器，而是一套 ROS 图：

- `detection` 负责图像到 `Detections`
- `tracking` 负责 `Detections` 到 `Tracklets`
- `decision` 负责 `Tracklets` 到 `Target`，并给控制逻辑提供取靶策略
- `locate` 负责里程计、IMU、云台 TF 的统一
- `serial` 负责和下位机通信
- `monitor` 负责可视化回传

## 模型与推理架构

检测器有两条实现路径。

1. Darknet 路径
   - 入口在 [detection/src/detection_node.cpp](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/src/detection/src/detection_node.cpp>)
   - 核心类在 [detection/src/detector.cpp](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/src/detection/src/detector.cpp>)
   - 通过 `parse_network_cfg_custom` 加载 `.cfg`，`load_weights` 加载 `.weights`
   - 推理后用 `get_network_boxes` 取框，再按概率选最佳类别并组装 ROS 消息

2. DeepStream 路径
   - 入口在 [detection/src/deepstream_node.cpp](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/src/detection/src/deepstream_node.cpp>)
   - 适配器在 [detection/src/deepstream_detector.cpp](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/src/detection/src/deepstream_detector.cpp>)
   - 通过 `deepstream_app_main` 启动 GStreamer/DeepStream 管线
   - 结果从 `NvDsBatchMeta` 和 `NvDsObjectMeta` 中取出后再发成 `Detections`

这套工程的“后处理”主要有两层：

- 检测框后处理：Darknet 路径中的 NMS、阈值筛选、类别决策
- 跟踪后处理：`tracking` 包使用 `motpy` 做多目标跟踪，把检测框变成稳定的 `Tracklets`

## 输入输出

主要 ROS 话题关系如下：

- 输入图像：`/camera/image_raw` 或 `image_in`
- 检测输出：`/detection/detections`
- 跟踪输出：`/tracking/tracklets`
- 决策输出：`/decision/target`
- 云台/控制状态：`/locate/*`、`/serial/*`

`monitor` 节点把图像、检测框和 tracklet 叠加回显，便于现场调试。

## 推理后端

工程支持两种推理后端：

- Darknet：适合训练、离线验证和本机调试
- DeepStream：适合 Jetson 部署，直接把视频流接入 NVIDIA 推理管线

关键配置在 [ros_ws/data/param-yolo.yaml](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/data/param-yolo.yaml>) 和 [ros_ws/data/deepstream_app_config.txt](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/data/deepstream_app_config.txt>)，前者决定 Darknet 模型路径，后者决定 DeepStream 流水线和 TensorRT 引擎文件。

## 跟踪与决策

`tracking` 包不是深度学习跟踪器，而是 Python 版多目标跟踪封装。它使用 `motpy`，按类别分别维护 tracker，并把 `Detection` 变成 `Tracklet`。关键实现见 [ros_ws/src/tracking/scripts/track.py](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/src/tracking/scripts/track.py>)。

`decision` 包提供两种取靶风格：

- `simple_tracker`：按敌方颜色、图像中心距离和 TF 直接取目标
- `weighted_tracker`：把类别、尺寸、位置、包含关系等因素打分，选得分最高的 tracklet

相关实现见 [ros_ws/src/decision/src/simple_tracker.cpp](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/src/decision/src/simple_tracker.cpp>)、[ros_ws/src/decision/src/weighted_tracker.cpp](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/src/decision/src/weighted_tracker.cpp>) 和 [ros_ws/src/decision/src/bounding_box.cpp](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/src/decision/src/bounding_box.cpp>)。

## 关键配置

- [ros_ws/data/param-camera.yaml](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/data/param-camera.yaml>)：相机内参、畸变和成像尺寸
- [ros_ws/data/param-yolo.yaml](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/data/param-yolo.yaml>)：Darknet 检测模型路径和阈值
- [ros_ws/data/param-yolo-tiny.yaml](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/data/param-yolo-tiny.yaml>)：轻量模型配置
- [ros_ws/data/param-decision.yaml](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/data/param-decision.yaml>)：敌方颜色和决策参数
- [ros_ws/data/param-localization.yaml](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/data/param-localization.yaml>)：robot_localization 的 EKF 输入
- [ros_ws/data/param-serial.yaml](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/data/param-serial.yaml>)：串口通信参数
- [ros_ws/data/deepstream-infer.txt](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/data/deepstream-infer.txt>)：DeepStream YOLO 解析、engine、label 和阈值
- [ros_ws/data/deepstream_app_config.txt](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/data/deepstream_app_config.txt>)：DeepStream 管线源、sink、mux、OSD、tracker

## 关键代码

- [ros_ws/src/detection/include/detector.hpp](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/src/detection/include/detector.hpp>)
- [ros_ws/src/detection/include/deepstream_detector.hpp](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/src/detection/include/deepstream_detector.hpp>)
- [ros_ws/src/detection/src/detector.cpp](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/src/detection/src/detector.cpp>)
- [ros_ws/src/detection/src/deepstream_detector.cpp](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/src/detection/src/deepstream_detector.cpp>)
- [ros_ws/src/monitor/src/video_monitor.cpp](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/src/monitor/src/video_monitor.cpp>)
- [ros_ws/src/locate/src/locate.cpp](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/src/locate/src/locate.cpp>)
- [ros_ws/src/serial/src/serial_interface.cpp](</home/cr/Codes/autoaim/docs/arch/robomaster-cv/ros_ws/src/serial/src/serial_interface.cpp>)

## 可复用建议

- 检测层可以直接复用 ROS2 节点封装、消息定义和 DeepStream 适配方式，尤其适合把现有 Darknet 模型迁移到 Jetson。
- `tracking` 的 motpy 方案适合作为“轻量 tracklet 层”，放在检测和决策之间，减少抖动。
- `weighted_tracker` 的打分思路适合做启发式取靶，但它强依赖目标类别与场景约束，移植时要重新标定权重。
- `locate` + `serial` 的 TF/消息分层清晰，适合作为自瞄框架的通信骨架。
- 若要继续扩展，建议把参数文件和 launch 进一步收敛成统一配置模板，减少不同节点的手工拼接。

