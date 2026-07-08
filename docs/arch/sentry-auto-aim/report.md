# sentry-auto-aim 模型架构报告

## 项目定位

`sentry-auto-aim` 是辽宁科技大学 COD 战队基于 FYT 2024 Vision Project 继续开发的 ROS2 Humble 视觉/自瞄仓库。仓库按 ROS2 package 拆分为 `rm_bringup`、`rm_auto_aim`、`rm_rune`、`rm_hardware_driver`、`rm_interfaces`、`rm_robot_description`、`rm_utils` 等模块。

该项目不是单一神经网络自瞄，而是两条视觉链路：

- 装甲板：传统灯条/装甲板几何检测 + ONNX 数字分类 + PnP/BA 姿态修正 + EKF 车辆状态跟踪 + 选板/弹道/火控。
- 能量机关：YOLOX 关键点模型 + OpenVINO 推理 + NMS/关键点解码 + R 标传统识别 + PnP/EKF/曲线拟合预测。

## 总体启动链路

`rm_bringup/launch/bringup.launch.py` 将相机、串口、装甲板 detector、装甲板 solver、能量机关 detector、能量机关 solver 组合启动。图像侧使用 `rclcpp_components` 的 `component_container_mt` 和 intra-process communication，降低图像拷贝开销。

主要数据流：

1. `hik_camera` 或视频节点发布 `image_raw` 与 `camera_info`。
2. `armor_detector` 订阅图像，发布 `armor_detector/armors`。
3. `armor_solver` 订阅装甲板结果，输出 `armor_solver/target`、`armor_solver/cmd_gimbal`。
4. `rune_detector` 订阅图像，发布 `rune_target`。
5. `rune_solver` 订阅能量机关目标，输出云台指令。
6. `rm_serial_driver` 与下位机通信，发送云台/开火控制，接收 IMU、模式、裁判系统等信息。

## 装甲板检测架构

装甲板检测入口在 `rm_auto_aim/armor_detector`。

### 图像预处理与灯条提取

`Detector::preprocessImage` 将输入图像转灰度后按 `binary_thres` 阈值二值化。`Detector::findLights` 对二值图做高斯模糊和轮廓提取，通过 `boundingRect`、`minAreaRect`、`fitLine` 估计灯条方向，再按面积、填充率、长宽比、倾角筛选候选灯条。

灯条颜色不是在预处理阶段直接分离，而是在候选轮廓 ROI 内统计红蓝通道和，根据 `sum_r > sum_b` 判断红/蓝，并结合 `detect_color` 保留敌方颜色。

### 装甲板配对

`Detector::matchLights` 将灯条按 x 坐标排序后两两配对。筛选逻辑包括：

- 两灯条之间不能包含额外灯条。
- 两灯条中心距离不能超过大装甲板距离上限。
- 灯条长度比例、中心距离、装甲板角度必须满足 `armor.*` 参数。

得到的 `Armor` 会被送入数字分类和角点修正。

### 数字分类模型

数字分类在 `NumberClassifier` 中完成，使用 OpenCV DNN 的 `cv::dnn::readNetFromONNX` 加载模型。仓库提供两个 ONNX：

- `models/armor_detector/lenet.onnx`
- `models/armor_detector/mlp.onnx`

README 写明主要使用 LeNet-5 结构。输入构造流程是：

1. 根据左右灯条角点做透视变换。
2. 小装甲板 warp 宽度 32，大装甲板 warp 宽度 54，高度 28。
3. 取中间 `20x25` ROI。
4. 灰度化、大津二值化、resize 到 `28x28`。
5. 归一化到 `0..1` 后 forward。

分类输出通过 `label.txt` 映射为装甲板数字/类别，并按 `classifier_threshold` 和 `ignore_classes` 过滤。代码还会过滤装甲板类型与数字不匹配的结果，例如大装甲不接受 `outpost/2/sentry`，小装甲不接受 `1/base`。

### 角点修正与 BA

项目在 FYT/rm_vision 基础上加入了：

- PCA 角点修正：`light_corner_corrector` 用灯条对称轴和亮度梯度寻找更稳定的上下角点。
- BA yaw 优化：`ba_solver` 使用 G2O 相关依赖优化装甲板 yaw，减少 PnP 多解和角点误差对朝向的影响。
- PnP 解选择：参数 `pnp_solution_selection` 用于选择更符合云台 pitch 约束的解。

这些逻辑由 `armor_detector_params.yaml` 中的 `use_pca`、`use_ba`、`pnp_solution_selection` 控制。

## 装甲板跟踪与火控

装甲板解算入口在 `rm_auto_aim/armor_solver`。

### EKF 状态

`ArmorTracker` 使用 9 维状态描述目标整车中心、速度、yaw 和半径：

```text
[xc, vxc, yc, vyc, za, vza, yaw, v_yaw, r]
```

观测量是单块装甲板的三维位置与 yaw：

```text
[xa, ya, za, yaw]
```

跟踪状态机包括 `DETECTING`、`TRACKING`、`TEMP_LOST`、`LOST`。初始化时选择离图像中心最近的装甲板，后续按相同数字 ID、空间距离、yaw 差进行关联。目标类型分为普通四装甲、前哨站三装甲、平衡步兵二装甲。

### 选板与弹道

`Solver::solve` 会叠加图像处理延迟、`prediction_delay` 和弹丸飞行时间，对目标中心和 yaw 做迭代预测。`selectBestArmor` 按目标 yaw 角速度分两种策略：

- 低转速：选取朝向相机夹角最小的装甲板，并带有锁定机制避免频繁切换。
- 高转速：使用 `coming_angle` / `leaving_angle` 的非对称角度窗口选择即将转入可击打区域的装甲板。

弹道补偿由 `rm_utils/math/trajectory_compensator` 提供，支持理想模型与带空气阻力模型。火控窗口按装甲板投影宽度、距离、`fire_margin`、最小/最大容差计算，只判断 yaw 是否落入射击窗口。

## 能量机关检测架构

能量机关检测入口在 `rm_rune/rune_detector`。

### 模型和输入输出

仓库提供 YOLOX 关键点模型：

- `models/rune_detector/yolox_rune.onnx`
- `models/rune_detector/yolox_rune.xml` + `yolox_rune.bin`
- `models/rune_detector/yolox_rune_3.6m.onnx`
- `models/rune_detector/yolox_rune_3.6m.xml` + `yolox_rune_3.6m.bin`

输入固定为 `480x480`，使用 letterbox 保持比例并记录逆变换矩阵。OpenVINO 通过 `ov::Core::read_model` 和 `compile_model` 加载模型，`device_type` 可配 `CPU/GPU/AUTO`。GPU 下代码会将 tensor precision 设置为 f16，其他设备为 f32。

输出在代码中按 `3549 x 21` 矩阵解析：

- 10 个数：5 个关键点坐标。
- 1 个数：confidence。
- 2 个数：颜色分类。
- 2 个数：类别分类。
- 其余维度由模型输出布局保留。

关键点包括 R 标中心和待击打扇叶四点。后处理使用 YOLOX stride `8/16/32` 网格解码、TopK、NMS，并对高度重合且置信度接近的候选做关键点平均融合。

### R 标传统识别

参数 `detect_r_tag` 开启时，网络预测的 R 标点会作为先验，`detectRTag` 在该点附近构造 `200x200` ROI，用大津二值化、膨胀、轮廓包含关系重新估计 R 标中心。这个逻辑用于提升 R 标稳定性。

## 能量机关预测

`rm_rune/rune_solver` 负责能量机关姿态与旋转预测：

- 使用 `PnPSolver` 将 5 个关键点解算为 rune pose。
- pose 转换到 `odom_vision` 后进入 4 维 EKF。
- `CurveFitter` 拟合小符/大符旋转曲线。
- `predictTarget` 用拟合曲线预测未来角度和击打点。
- `solveGimbalCmd` 用弹道补偿计算 yaw/pitch，并根据目标半径和距离给出开火建议。

## 配置结构

已复制的关键配置位于 `configs/`：

- `launch_params.yaml`：控制命名空间、相机/视频、虚拟串口、是否启动 rune 和导航。
- `node_params/armor_detector_params.yaml`：灯条、装甲板、PCA、BA、数字分类阈值。
- `node_params/armor_solver_params.yaml`：EKF 噪声、跟踪阈值、选板、预测延迟、弹道和火控参数。
- `node_params/rune_detector_params.yaml`：YOLOX 模型路径、OpenVINO 设备、异步请求数量、R 标传统识别参数。
- `node_params/rune_solver_params.yaml`：能量机关预测、弹道、EKF 参数。
- `node_params/serial_driver_params.yaml`：串口设备、协议类型、时间戳偏移。
- `camera_info.yaml`、`camera/camera_params.yaml`：相机内参与曝光/增益。
- `robot_description/*.xacro`：相机、云台、哨兵坐标系。

## 关键代码归档

已复制的关键源码位于 `code/`：

- `rm_auto_aim/armor_detector/`：传统灯条/装甲板检测、数字分类、PCA 角点修正、BA yaw 优化。
- `rm_auto_aim/armor_solver/`：目标 EKF、选板、弹道补偿、火控窗口。
- `rm_rune/rune_detector/`：OpenVINO YOLOX 推理、关键点解码、NMS、R 标传统识别。
- `rm_rune/rune_solver/`：PnP、EKF、曲线拟合预测、能量机关云台指令。
- `rm_utils/math/`：通用 EKF、PnP、弹道补偿。
- `rm_hardware_driver/rm_serial_driver/`：串口协议与上下位机数据交换。
- `rm_hardware_driver/ros2_hik_camera/`：海康相机 ROS2 节点入口。

## 可复用建议

- 如果当前项目需要 ROS2 组件化架构，可以直接参考 `rm_bringup` 的节点组合方式和 `rm_interfaces` 的消息拆分。
- 装甲板检测仍是传统 CV 主链路，适合低算力、可解释、便于现场调参的方案；数字分类的 LeNet/MLP 可作为轻量分类器参考。
- 能量机关 YOLOX/OpenVINO 代码较完整，后处理清晰，适合复用到 x86 NUC 或支持 OpenVINO 的平台。
- `armor_solver` 的选板和火控逻辑比基础 tracker 更接近实战，可重点看 `selectBestArmor`、`isOnTarget`、`trajectory_compensator`。
- 若要迁移到非 ROS 工程，需要重写节点通信、TF、参数读取和消息类型；核心 detector/solver 算法可以拆出来复用。

