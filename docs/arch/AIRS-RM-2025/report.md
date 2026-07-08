# AIRS-RM-2025 架构报告

## 项目定位

这是一个完整的 Python 自瞄闭环工程，面向 RoboMaster EP/Core 和 Atlas 200I DK 平台。它把模型推理、检测后处理、目标跟踪、目标选择、云台控制和串口任务分发串成了一条链路。

从仓库内容看，模型本体并未以源码形式提供，只有一个 `16.om` 昇腾离线模型文件和 `labels.txt` 标签文件。因此可以确认的是推理后端是昇腾 `ais_bench`，但网络结构细节没有在源码里公开。

## 模型架构

可见信息只能推断出这是一个 YOLO 风格的目标检测模型：

- `autoaim.py` 通过 `ais_bench.infer.interface.InferSession` 加载 `16.om`。
- `detector.py` 对模型输出做 `nms()`，并把输出当作 `[xyxy, conf, cls]` 风格的检测框处理。
- `det_utils.py` 的 `non_max_suppression()` 接口与 YOLO 系列后处理兼容。

结合 `labels.txt`，当前类别为 4 类：

- `RedArmor`
- `RedGimbal`
- `BlueArmor`
- `BlueGimbal`

更合理的结论是：模型是一个装甲板/云台部件检测器，是否含有额外的关键点或角度回归分支，仓库里没有明确证据。

## 输入输出

输入：

- 来自 USB 摄像头的 BGR 图像。
- `640x640` 的 letterbox 预处理输入。
- 串口任务指令，决定攻击红方还是蓝方。

输出：

- 检测框、类别、置信度。
- 卡尔曼滤波后的跟踪状态。
- 预测瞄准点、归一化速度、锁定目标信息。
- RoboMaster 云台 yaw/pitch 速度指令和开火指令。

## 推理后端

推理后端是昇腾 Atlas 200I DK 的 `ais_bench`：

- `InferSession(0, model_path)` 加载 `.om` 模型。
- 线程内完成模型初始化和预热。
- 输入数据为归一化后的 `float32` 图像张量。

这说明部署路径是“离线 OM 模型 + 昇腾 NPU 推理”，不是 PyTorch 原地推理，也不是纯 OpenCV 传统视觉。

## 后处理 / 跟踪 / 预测

这部分是该仓库最完整的地方。

### 检测后处理

`detector.py` 负责：

- 调用 `nms()` 过滤重复框。
- `scale_coords()` 把 640 输入尺度映射回原图。
- 过滤掉部分类别，例如模型类别 `1`、`3` 不进入自瞄跟踪链路。
- 利用 HSV 颜色判定进一步把目标细分为红/蓝。

### 跟踪

`tracker.py` 使用 `filterpy.KalmanFilter` 维护 `[x, y, vx, vy]` 状态。

### 目标选择

`detector.py` 里有一个加权评分器：

- 距离权重 `WEIGHT_DISTANCE`
- 角度权重 `WEIGHT_ANGLE`
- 尺寸权重 `WEIGHT_SIZE`

并带有“粘性锁定”逻辑，避免目标频繁切换。

### 运动预测

`Track.calculate_aim_point()` 会根据速度做前馈预测：

- 速度很小时直接瞄准中心点，减少抖动。
- 速度足够大时按 `AIM_PREDICTION_FRAMES` 向前外推。

## 控制接口

`autoaim.py` 通过 `robomaster` SDK 控制云台和发射器：

- `ep_robot.gimbal.drive_speed(...)`
- `ep_robot.gimbal.moveto(...)`
- `ep_blaster.fire(times=1)`

状态机包括：

- `IDLE`
- `SEARCHING`
- `TRACKING`
- `COASTING`

串口入口在 `serial_listener.py`，收到 `1` 或 `2` 后分别调起蓝方或红方任务脚本。

## 关键配置

已复制的关键配置文件：

- `code/config.py`
- `configs/start.sh`
- `models/labels.txt`

`config.py` 是核心调参中心，主要包含：

- 跟踪稳定阈值、开火冷却、预测帧数
- Kalman 滤波参数
- IOU 匹配阈值
- 目标选择权重
- 云台 PID 初值
- HSV 颜色范围
- 模型路径和输入尺寸
- 攻击目标白名单

## 关键代码

已复制的关键源码：

- `code/autoaim.py`
- `code/detector.py`
- `code/tracker.py`
- `code/det_utils.py`
- `code/utils.py`
- `code/config.py`
- `code/serial_listener.py`
- `code/main_shoot_blue.py`
- `code/main_shoot_red.py`
- `code/Calcu_distance_api.py`
- `code/GetZDgree_api.py`

关键观察：

- `autoaim.py` 是总编排入口，负责线程、状态机、摄像头、模型和 SDK 控制。
- `detector.py` 把“检测 + 跟踪 + 目标选择 + 颜色确认 + 预测点输出”整合在一起。
- `tracker.py` 只做状态估计，不负责检测。
- `serial_listener.py` 更像任务调度器，不是核心算法。

## 可复用建议

可直接复用的部分：

- `detector.py` 的目标评分、白名单和粘性锁定逻辑。
- `tracker.py` 的 Kalman 跟踪器结构。
- `autoaim.py` 的线程分工、状态机和云台控制组织方式。
- `config.py` 的集中参数化方式。

需要适配的部分：

- `16.om` 的真实输出格式需要按实际模型确认。
- `robomaster` SDK、串口协议、云台零位和 PID 参数都要按硬件平台重标定。
- `labels.txt` 的类别含义要和训练数据一致，否则颜色和类别映射会错。

仅作参考的部分：

- 距离和旋转角度估算函数目前比较轻量，更像工程启发式，不是严格弹道模型。
- `main_shoot_blue.py` 和 `main_shoot_red.py` 里有演示性质的固定时长任务逻辑。

## 结论

这是一个可运行的闭环自瞄工程，技术栈清晰，模型推理、检测后处理、跟踪和控制接口都已串联起来。模型结构本体未公开，但从 `ais_bench + NMS + labels` 可以明确它是昇腾侧的 YOLO 风格检测模型部署工程。
