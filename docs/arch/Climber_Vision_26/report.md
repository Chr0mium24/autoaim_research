# Climber_Vision_26 架构报告

## 项目定位

`Climber_Vision_26` 是一个完整的 RoboMaster 自瞄视觉工程，基于同济 `sp_vision_25` 深度改造，覆盖了主相机自瞄、打符、全向感知、相机标定、串口/C板通信和部署脚本。

本次整理已复制的关键内容：

- 配置：[`configs/`](configs/)
- 代码：[`code/`](code/)
- 模型与轻量资产：[`models/`](models/)

说明：本目录只保留了模型结构文件和小体积资产，未复制 `.bin` 权重等大文件。

## 模型与推理架构

### 1. 主自瞄检测

主入口是 `tasks/auto_aim/yolo.cpp`，通过 `configs/*.yaml` 里的 `yolo_name` 动态选择实现：

- `YOLOV5`
- `YOLOV8`
- `YOLO11`

三者都使用 OpenVINO 推理，输入前做统一的 BGR 归一化、resize letterbox，输出后做 NMS，再生成 `Armor`。

输入输出要点：

- 输入：BGR 图像，内部缩放到 `640x640` 或 `416x416`
- 输出：装甲板类别、置信度、bbox、角点、中心点归一化坐标
- 后处理：NMS + 类别过滤 + 装甲类型过滤

差异点：

- `YOLOV5`：`13` 类，输出里包含颜色/数字独热信息
- `YOLOV8`：`2` 类，依赖分类器补充名字
- `YOLO11`：`38` 类，直接输出装甲名字和四点

### 2. 传统装甲板检测

`tasks/auto_aim/detector.cpp` 保留了传统灯条方案：

- 灰度化
- 阈值分割
- 轮廓提取
- 灯条几何筛选
- 双灯条配对成装甲板
- 分类器识别数字

它更像是网络检测的兜底实现，也用于调参和对比实验。

### 3. 跟踪与状态估计

`tasks/auto_aim/tracker.cpp` + `tasks/auto_aim/target.cpp` 负责从检测框进入稳定目标跟踪。

核心状态机：

- `lost`
- `detecting`
- `tracking`
- `temp_lost`
- `switching`

核心估计器是 EKF，状态围绕目标中心、速度、偏航、角速度、半径等展开。

特殊处理：

- 前哨站有三装甲板关联和重锚定逻辑
- 目标发散会强制回到 `lost`
- NIS 失败率过高也会丢弃当前目标

### 4. 弹道与瞄准

`tasks/auto_aim/solver.cpp` 完成相机位姿解算和世界坐标重投影，`tasks/auto_aim/aimer.cpp` 负责火控角度计算。

流程是：

1. `solvePnP` 求装甲板位姿
2. 转到云台/世界坐标
3. 用 `AirResistTrajectory` 迭代求子弹飞行时间
4. 重新预测目标
5. 输出 `yaw/pitch`

`Aimer` 还做了目标板切换判断、发射间隔控制和前哨站俯仰补偿。

### 5. 打符链路

`tasks/auto_buff/` 是独立链路，配置里通常使用：

- `model: assets/buff_repvgg.xml`
- `pnp_mode: 4i`
- `buff_target` / `fire_gap_time` / `predict_time`

打符链路包含：

- `YOLO11_BUFF` 检测关键点
- `Buff_Detector` 做候选筛选、R 中心细化、去重和小符关联
- `Solver` 做靶面 PnP、靶心/扇叶重投影
- `Target` 用 EKF 维护小符/大符运动状态
- `Aimer` 负责出枪角度，支持线性空气阻力

大符还引入了 `BigTargetGroup` 和 `BigTargetSelector`，做 5 槽管理和目标槽位选择。

### 6. 全向感知

`tasks/omniperception/` 负责多 USB 相机的轮询推理和目标优先级筛选。

- `Perceptron`：多相机轮询 + OpenVINO 推理 + 队列缓存
- `ArmorSelector`：颜色过滤、优先级排序、无敌目标过滤
- `Decider`：把多相机检测结果转换成云台角度指令

这条链路是给哨兵/感知场景补充视野的，不是主自瞄必须项。

## 推理后端

工程使用 OpenVINO：

- `CMakeLists.txt` 中固定了 OpenVINO 2026.1 路径
- `YOLOV5/YOLOV8/YOLO11/YOLO11_BUFF` 都是 `ov::CompiledModel`
- `device` 由 YAML 指定，支持 `CPU` / `GPU`

配置里最关键的推理字段：

- `yolo_name`
- `device`
- `*_model_path`
- `use_roi`
- `use_traditional`

## 输入输出链路

### 主自瞄

- 输入：工业相机或 USB 相机图像 + 云台四元数
- 输出：`io::Command`，包含 `control`、`shoot`、`yaw`、`pitch`

### 打符

- 输入：图像 + 云台姿态 + 时间戳
- 输出：`PowerRune`、`Target`、最终火控 `Plan`

### 全向感知

- 输入：多路 USB 相机图像
- 输出：`DetectionResult`，包含装甲列表和偏移角

## 关键配置

已复制的配置文件见 [`configs/`](configs/)。

最关键的是这些：

- [`configs/infantry.yaml`](configs/infantry.yaml)
- [`configs/infantry_4.yaml`](configs/infantry_4.yaml)
- [`configs/infantry_5.yaml`](configs/infantry_5.yaml)
- [`configs/sentry.yaml`](configs/sentry.yaml)
- [`configs/uav.yaml`](configs/uav.yaml)
- [`configs/test_aim.yaml`](configs/test_aim.yaml)
- [`configs/test_buff.yaml`](configs/test_buff.yaml)
- [`configs/calibration.yaml`](configs/calibration.yaml)

核心字段含义：

- `yolo_name`：选择 `yolov5` / `yolov8` / `yolo11`
- `yolov*_model_path`：OpenVINO 模型路径
- `threshold` / `min_confidence`：检测阈值
- `use_traditional`：是否对网络输出做传统几何二次修正
- `R_camera2gimbal` / `t_camera2gimbal`：手眼外参
- `air_resistance_k`：线性阻力系数
- `comming_angle` / `leaving_angle` / `decision_speed`：火控切板逻辑
- `mode`、`camera_name_map`、`perception_device`：全向感知和多相机配置

## 关键代码

建议优先看这些复制出来的文件：

- [`code/tasks/auto_aim/yolo.cpp`](code/tasks/auto_aim/yolo.cpp)
- [`code/tasks/auto_aim/yolos/yolo11.cpp`](code/tasks/auto_aim/yolos/yolo11.cpp)
- [`code/tasks/auto_aim/yolos/yolov5.cpp`](code/tasks/auto_aim/yolos/yolov5.cpp)
- [`code/tasks/auto_aim/yolos/yolov8.cpp`](code/tasks/auto_aim/yolos/yolov8.cpp)
- [`code/tasks/auto_aim/detector.cpp`](code/tasks/auto_aim/detector.cpp)
- [`code/tasks/auto_aim/tracker.cpp`](code/tasks/auto_aim/tracker.cpp)
- [`code/tasks/auto_aim/target.cpp`](code/tasks/auto_aim/target.cpp)
- [`code/tasks/auto_aim/solver.cpp`](code/tasks/auto_aim/solver.cpp)
- [`code/tasks/auto_aim/aimer.cpp`](code/tasks/auto_aim/aimer.cpp)
- [`code/tasks/auto_aim/planner/planner.cpp`](code/tasks/auto_aim/planner/planner.cpp)
- [`code/tasks/auto_buff/yolo11_buff.cpp`](code/tasks/auto_buff/yolo11_buff.cpp)
- [`code/tasks/auto_buff/buff_detector.cpp`](code/tasks/auto_buff/buff_detector.cpp)
- [`code/tasks/auto_buff/buff_target.cpp`](code/tasks/auto_buff/buff_target.cpp)
- [`code/tasks/auto_buff/buff_solver.cpp`](code/tasks/auto_buff/buff_solver.cpp)
- [`code/tasks/auto_buff/buff_aimer.cpp`](code/tasks/auto_buff/buff_aimer.cpp)
- [`code/tasks/omniperception/perceptron.cpp`](code/tasks/omniperception/perceptron.cpp)
- [`code/tasks/omniperception/decider.cpp`](code/tasks/omniperception/decider.cpp)
- [`code/io/camera.cpp`](code/io/camera.cpp)
- [`code/io/cboard.cpp`](code/io/cboard.cpp)

## 可复用建议

对现有自瞄工程可直接复用的部分：

- OpenVINO 统一推理封装，便于同一套代码切不同模型
- `Tracker` 的状态机和 outpost 特化逻辑
- `Aimer` 的阻力弹道迭代和切板抑制开火
- `Buff_Detector` 的 R 中心细化和候选去重
- `Perceptron` 的多相机轮询和结果队列

需要适配的部分：

- 角点语义和类别数
- 相机内外参
- `R_camera2gimbal`、`R_gimbal2imubody`
- 各机器人型号的 `air_resistance_k`、延迟和开火阈值

仅建议参考的部分：

- `scripts/` 下的人机调试工具
- 具体兵种 YAML 里的经验参数
- `tinympc` 的调参值

