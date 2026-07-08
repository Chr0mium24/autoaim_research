# TGU_Vision_2026 架构报告

## 项目定位

`TGU_Vision_2026` 是同济 `sp_vision_25` 的 RM2026 继续演化版本，目标是给 TGU304 提供可部署的自瞄、打符、全向感知和 MPC 控制框架。

本次整理已复制的关键内容：

- 配置：[`configs/`](configs/)
- 代码：[`code/`](code/)
- 模型与轻量资产：[`models/`](models/)

说明：本目录只保留了模型结构文件和小体积资产，没有复制 `.bin` 权重和大型 demo 资源。

## 模型与推理架构

### 1. 主自瞄检测

`tasks/auto_aim/yolo.cpp` 通过 `yolo_name` 选择：

- `YOLOV5`
- `YOLOV8`
- `YOLO11`

这些实现都走 OpenVINO，输入是 BGR 图像，输出是装甲板检测结果和角点。

输出语义：

- `YOLOV5`：颜色 + 数字 + 4 角点
- `YOLOV8`：2 类输出 + 4 角点
- `YOLO11`：38 类输出 + 4 角点

检测后都会做：

- resize / letterbox
- OpenVINO 推理
- NMS
- 类别过滤
- 必要时用传统灯条法二次修正角点

### 2. 打符关键点模型

打符使用 `YOLO11_BUFF`，配置通常是：

- `model: assets/yolo11_buff_int8.xml`

这个模型是 OpenVINO 下的关键点检测器，输出 9 个原始关键点，再 remap 成 6 点 PnP 语义：

- 上边中点
- 左边中点
- 下边中点
- 右边中点
- 扇叶中心
- R 中心

这和旧版“灯臂外推 R 中心”的语义不同，后端 solver 也专门做了兼容。

### 3. 多相机全向感知

`omniperception::Perceptron` 负责多 USB 相机轮询推理：

- 读取 `camera_name_map`
- 按相机顺序 round-robin
- 推理后放入 `DetectionResult` 队列
- `Decider` 再把它转成云台角度或导航动作

### 4. MPC 规划

`tasks/auto_aim/planner/planner.cpp` 使用 TinyMPC：

- 状态：yaw / yaw_vel / pitch / pitch_vel
- 时域：`HORIZON = 100`
- 步长：`DT = 0.01`
- 约束：`max_yaw_acc` / `max_pitch_acc`

它不是简单 P 控制，而是先根据目标预测生成参考轨迹，再分别解 yaw 和 pitch 的二次优化。

## 推理后端

工程默认是 OpenVINO：

- `CMakeLists.txt` 固定了 OpenVINO 2024.6 路径
- 推理设备由 YAML 指定，常见是 `GPU` 或 `CPU`
- `MultiThreadDetector` 使用 `THROUGHPUT` 模式异步推理
- 主检测默认更偏 `LATENCY`

## 输入输出链路

### 主自瞄

- 输入：工业相机图像 + 云台状态 + 子弹初速
- 输出：`Plan`，包含 `yaw/pitch`、速度、加速度和 `fire`

### 打符

- 输入：图像 + 云台状态 + 时间戳
- 输出：`PowerRune`、`BigTarget` / `SmallTarget`、最终 `Plan`

### 全向感知

- 输入：多路 USB 相机图像
- 输出：`DetectionResult` 队列，包含偏航/俯仰角偏移

## 关键配置

已复制的配置文件见 [`configs/`](configs/)。

重点查看：

- [`configs/standard.yaml`](configs/standard.yaml)
- [`configs/sentry.yaml`](configs/sentry.yaml)
- [`configs/demo.yaml`](configs/demo.yaml)
- [`configs/example.yaml`](configs/example.yaml)
- [`configs/ascento.yaml`](configs/ascento.yaml)
- [`configs/calibration.yaml`](configs/calibration.yaml)
- [`configs/camera.yaml`](configs/camera.yaml)
- [`configs/buff_layout.xml`](configs/buff_layout.xml)
- [`configs/mpc_layout.xml`](configs/mpc_layout.xml)

核心字段含义：

- `yolo_name`：选择具体 YOLO 版本
- `classify_model`：数字分类器
- `yolo*_model_path`：OpenVINO 检测模型路径
- `device`：推理设备
- `use_traditional`：是否混用传统灯条法修角点
- `perception_device`：全向感知的推理设备
- `omni_memory_duration_ms`：全向感知记忆窗口
- `buff_pitch_weight`：大符槽位选择时 pitch 代价权重
- `fire_thresh`、`Q_yaw`、`Q_pitch`、`R_yaw`、`R_pitch`：MPC 代价与约束

## 关键代码

建议优先看这些复制出来的文件：

- [`code/tasks/auto_aim/yolo.cpp`](code/tasks/auto_aim/yolo.cpp)
- [`code/tasks/auto_aim/yolos/yolo11.cpp`](code/tasks/auto_aim/yolos/yolo11.cpp)
- [`code/tasks/auto_aim/yolos/yolov5.cpp`](code/tasks/auto_aim/yolos/yolov5.cpp)
- [`code/tasks/auto_aim/yolos/yolov8.cpp`](code/tasks/auto_aim/yolos/yolov8.cpp)
- [`code/tasks/auto_aim/tracker.cpp`](code/tasks/auto_aim/tracker.cpp)
- [`code/tasks/auto_aim/target.cpp`](code/tasks/auto_aim/target.cpp)
- [`code/tasks/auto_aim/solver.cpp`](code/tasks/auto_aim/solver.cpp)
- [`code/tasks/auto_aim/aimer.cpp`](code/tasks/auto_aim/aimer.cpp)
- [`code/tasks/auto_aim/planner/planner.cpp`](code/tasks/auto_aim/planner/planner.cpp)
- [`code/tasks/auto_aim/multithread/mt_detector.cpp`](code/tasks/auto_aim/multithread/mt_detector.cpp)
- [`code/tasks/auto_aim/multithread/commandgener.cpp`](code/tasks/auto_aim/multithread/commandgener.cpp)
- [`code/tasks/auto_buff/yolo11_buff.cpp`](code/tasks/auto_buff/yolo11_buff.cpp)
- [`code/tasks/auto_buff/buff_detector.cpp`](code/tasks/auto_buff/buff_detector.cpp)
- [`code/tasks/auto_buff/buff_big_group.cpp`](code/tasks/auto_buff/buff_big_group.cpp)
- [`code/tasks/auto_buff/buff_target.cpp`](code/tasks/auto_buff/buff_target.cpp)
- [`code/tasks/auto_buff/buff_solver.cpp`](code/tasks/auto_buff/buff_solver.cpp)
- [`code/tasks/auto_buff/buff_aimer.cpp`](code/tasks/auto_buff/buff_aimer.cpp)
- [`code/tasks/omniperception/perceptron.cpp`](code/tasks/omniperception/perceptron.cpp)
- [`code/tasks/omniperception/decider.cpp`](code/tasks/omniperception/decider.cpp)
- [`code/io/ros2/`](code/io/ros2/)
- [`code/io/gimbal/gimbal_nav.cpp`](code/io/gimbal/gimbal_nav.cpp)

## 可复用建议

可直接复用的部分：

- OpenVINO + 统一 YOLO wrapper
- 关键点打符模型的 9 点 -> 6 点语义映射
- 轮询式多相机全向感知
- `Tracker` 的状态机和 ROS2/全向感知切换逻辑
- TinyMPC 的 yaw/pitch 双通道规划

需要适配的部分：

- `camera_name_map` 中的相机方位和翻转参数
- `R_camera2gimbal` / `R_gimbal2imubody`
- `buff_target` 里的 EKF 噪声与发散阈值
- `buff_pitch_weight`、`omni_memory_duration_ms`
- 相机曝光、增益和串口/CAN 设备名

仅建议参考的部分：

- `code/readme.md`、`code/readme_spvision.md`
- `configs/` 中不同兵种的经验参数
- `buffers` / `layouts` 类可视化文件

