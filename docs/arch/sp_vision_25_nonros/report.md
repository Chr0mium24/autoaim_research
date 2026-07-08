# sp_vision_25_nonros 架构报告

## 项目定位

这是同济 SuperPower 25 赛季的非 ROS 自瞄框架。项目目标是用一套纯 C++ 代码跑通相机采集、装甲板识别、位姿解算、目标跟踪、弹道补偿、开火控制和打符流程；ROS 只作为可选接口，不是主依赖。

项目总体上分成三层：

- `io`：相机、云台、C 板、IMU、串口、ROS2 适配
- `tasks`：自瞄、打符、全向感知、规划器
- `tools`：EKF、轨迹、日志、线程队列、RANSAC、绘图等通用工具

## 模型与推理架构

自瞄视觉层有两条检测路线。

1. 传统视觉路线
   - `tasks/auto_aim/detector.cpp` 先灰度化、阈值化、找轮廓，再提灯条、拼装甲板
   - 用几何约束过滤灯条和装甲板，再裁剪图案送分类器
   - 优点是可解释，适合作为弱光或低算力备选

2. 神经网络路线
   - `tasks/auto_aim/yolo.cpp` 根据 `yolo_name` 动态选择 `YOLOV5 / YOLOV8 / YOLO11`
   - 三个后端都走 OpenVINO runtime，输入是固定尺寸 BGR 图
   - `YOLOV5` 输出的是装甲板框 + 4 点关键点 + 颜色/数字分支
   - `YOLOV8` 和 `YOLO11` 也是框 + 关键点，但后处理格式不同
   - 网络结果后面会进入传统几何检查和分类器复核

辅助分类器是独立的 ONNX 模型：

- [assets/tiny_resnet.onnx](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/assets/tiny_resnet.onnx>) 负责图案分类
- `classifier.cpp` 同时实现了 OpenCV DNN 和 OpenVINO 两套加载方式，默认走 OpenVINO 编译模型

打符模块则是另一套模型：

- `tasks/auto_buff/yolo11_buff.cpp` 使用 `yolo11_buff_int8.xml` 做能量机关扇叶/中心检测
- 后续由 `buff_target`、`buff_solver`、`buff_aimer` 完成旋转方向、扇叶预测和射击控制

## 输入输出

主自瞄链路的数据流是：

相机帧 -> `YOLO` 或传统 `Detector` -> `Armor` -> `Solver` -> `Target` -> `Aimer` / `Planner` -> `io::Command` -> C 板 / 云台。

打符链路的数据流是：

相机帧 -> `Buff_Detector` -> `PowerRune` -> `buff_solver` -> `buff_target` -> `buff_aimer` / `Planner` -> `io::Command`。

项目里有多个可执行入口：

- `standard` / `mt_standard`：日常自瞄主程序
- `standard_mpc` / `auto_aim_debug_mpc`：带 MPC/调试路径
- `sentry*`：哨兵模式，结合 ROS2 进行导航通信
- `uav*`：无人机相关入口
- `auto_buff*`：打符程序

## 推理后端

`YOLOV5/8/11` 都基于 OpenVINO。

- 载入方式：`core_.read_model(...)`
- 输入预处理：`PrePostProcessor` 设置 NHWC BGR -> NCHW RGB，归一化到 0-1
- 推理方式：同步 `infer()`，多线程模式下用 `start_async()` + 队列
- 输出后处理：`cv::dnn::NMSBoxes`、关键点排序、类别过滤、ROI 偏移修正

模型选择由 YAML 控制，见 [configs/example.yaml](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/configs/example.yaml>)、[configs/sentry.yaml](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/configs/sentry.yaml>)、[configs/standard3.yaml](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/configs/standard3.yaml>) 等。

## 后处理、跟踪和预测

检测后处理不只做 NMS，还会做一轮结构化校验：

- 传统 detector 会检查灯条角度、长宽比、装甲板矩形误差
- 网络 detector 会再走图案分类器，筛掉不合法的兵种组合
- `Armor` 对象中保存了世界系和云台系坐标、角度、类型和置信度

跟踪器是项目的核心资产之一。

- `Tracker` 维护 `lost / detecting / tracking / temp_lost / switching` 状态机
- `Target` 用 11 维 EKF 建模目标中心、速度、角速度、半径和高度
- 对不同兵种，初始化协方差和丢失阈值不同
- 高速陀螺、前哨站、基地都有单独分支

弹道和射击决策也被显式建模：

- `Aimer` 根据当前目标预测未来时刻的位置，再迭代解飞行时间
- `Planner` 使用 TinyMPC，把 yaw/pitch 当作二阶系统做轨迹优化
- 打符链路里 `buff_aimer` 还会结合旋转方向投票、扇叶切换抑制和开火间隔

## 关键配置

- [configs/example.yaml](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/configs/example.yaml>)：最完整的自瞄示例配置
- [configs/demo.yaml](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/configs/demo.yaml>)：演示模式，偏向 CPU 与传统视觉
- [configs/standard3.yaml](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/configs/standard3.yaml>)、[configs/standard4.yaml](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/configs/standard4.yaml>)：步兵实机配置
- [configs/sentry.yaml](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/configs/sentry.yaml>)：哨兵模式，含双相机和左右偏置
- [configs/uav.yaml](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/configs/uav.yaml>)：无人机配置
- [configs/ascento.yaml](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/configs/ascento.yaml>)：另一套实机标定和打符参数
- [configs/camera.yaml](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/configs/camera.yaml>)：相机类型切换
- [configs/calibration.yaml](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/configs/calibration.yaml>)：标定采集与 C 板通讯参数

这些配置里最关键的字段有：

- `yolo_name`：选择 `yolov5 / yolov8 / yolo11`
- `device`：OpenVINO 运行设备，常见 `CPU` / `GPU`
- `use_traditional`：是否用传统 detector 二次修正
- `use_roi` / `roi`：是否裁剪 ROI
- `camera_matrix` / `distort_coeffs` / `R_camera2gimbal`：几何链路标定
- `yaw_offset` / `pitch_offset` / `comming_angle` / `leaving_angle`：瞄准和切换阈值
- `buff_detector` 和 `planner` 段：打符链路参数

## 关键代码

自瞄主干：

- [tasks/auto_aim/yolo.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/tasks/auto_aim/yolo.cpp>)
- [tasks/auto_aim/yolos/yolov5.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/tasks/auto_aim/yolos/yolov5.cpp>)
- [tasks/auto_aim/yolos/yolov8.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/tasks/auto_aim/yolos/yolov8.cpp>)
- [tasks/auto_aim/yolos/yolo11.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/tasks/auto_aim/yolos/yolo11.cpp>)
- [tasks/auto_aim/detector.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/tasks/auto_aim/detector.cpp>)
- [tasks/auto_aim/tracker.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/tasks/auto_aim/tracker.cpp>)
- [tasks/auto_aim/solver.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/tasks/auto_aim/solver.cpp>)
- [tasks/auto_aim/aimer.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/tasks/auto_aim/aimer.cpp>)
- [tasks/auto_aim/target.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/tasks/auto_aim/target.cpp>)

打符主干：

- [tasks/auto_buff/buff_detector.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/tasks/auto_buff/buff_detector.cpp>)
- [tasks/auto_buff/buff_solver.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/tasks/auto_buff/buff_solver.cpp>)
- [tasks/auto_buff/buff_target.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/tasks/auto_buff/buff_target.cpp>)
- [tasks/auto_buff/buff_aimer.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/tasks/auto_buff/buff_aimer.cpp>)
- [tasks/auto_buff/yolo11_buff.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/tasks/auto_buff/yolo11_buff.cpp>)

规划器与多线程：

- [tasks/auto_aim/planner/planner.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/tasks/auto_aim/planner/planner.cpp>)
- [tasks/auto_aim/multithread/mt_detector.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/tasks/auto_aim/multithread/mt_detector.cpp>)
- [tasks/auto_aim/multithread/commandgener.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/tasks/auto_aim/multithread/commandgener.cpp>)

硬件与通信：

- [io/camera.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/io/camera.cpp>)
- [io/hikrobot/hikrobot.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/io/hikrobot/hikrobot.cpp>)
- [io/mindvision/mindvision.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/io/mindvision/mindvision.cpp>)
- [io/usbcamera/usbcamera.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/io/usbcamera/usbcamera.cpp>)
- [io/cboard.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/io/cboard.cpp>)
- [io/gimbal/gimbal.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/io/gimbal/gimbal.cpp>)
- [io/ros2/ros2.cpp](</home/cr/Codes/autoaim/docs/arch/sp_vision_25_nonros/io/ros2/ros2.cpp>)

## 可复用建议

- 这套工程最值得复用的是“检测 -> 几何校验 -> EKF 跟踪 -> 弹道预测 -> 控制”这一整条闭环，而不是单独拿一个模型。
- OpenVINO + `PrePostProcessor` 的封装比较干净，适合在 Intel 平台上直接复用。
- `Tracker` 的状态机和 `Target` 的 EKF 设计已经把兵种差异显式化，迁移时只要重配参数就能得到一个可工作的基线。
- `Planner` 和 TinyMPC 适合需要更强控制感的场景，但它比纯预测开火更重，调参成本也更高。
- 打符链路有自己独立的目标模型和预测流程，适合作为单独任务模块保留，不建议和自瞄主链路硬耦合。

