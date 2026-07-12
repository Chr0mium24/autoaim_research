# breeze 模型报告

## 1. 模型清单

本目录没有视觉/自瞄模型资产。[models/README.md](models/README.md) 明确说明本项目不包含视觉/自瞄模型本体，只保留控制框架、CAN、遥控和电机/舵机接口。

已检查 [report.md](report.md)、[README.md](README.md)、`models/`、`configs/` 和 `code/` 后，未发现 ONNX、TensorRT、OpenVINO、Darknet、Caffe、OpenCV DNN、相机读取或目标检测代码。

## 2. 模型细节

无可报告的模型格式、输入尺寸、输出布局、类别或关键点定义。

本项目不是视觉识别端，而是 Zephyr-RTOS 下的嵌入式设备与控制框架。

## 3. 预处理、后处理与控制逻辑

无图像预处理、检测后处理、NMS、PnP、跟踪或弹道预测逻辑。

可确认的控制/设备逻辑包括：

- [remote/dr16_remote.c](code/remote/dr16_remote.c)：解析 DR16/DT7 遥控器通道、开关、鼠标和按键状态。
- [can_rx_manager.c](code/can_rx_manager/can_rx_manager.c)：CAN 接收软件过滤、回调分发和负载统计。
- [can_tx_manager.c](code/can_tx_manager/can_tx_manager.c)：CAN 发送者注册、事件触发和周期发送。
- [motor_dji/can_dji.c](code/motor_dji/can_dji.c)：DJI 电机反馈解析、目标电流序列化和心跳离线检测。
- [servo_motor_pwm.c](code/servo_motor_pwm/servo_motor_pwm.c)：PWM 舵机角度、偏置和限位控制。

如果上层接视觉自瞄，视觉系统需要另行输出 yaw/pitch/distance/目标状态，本项目负责把控制量落到 CAN 电机、PWM 舵机或其他板级接口。

## 4. 部署后端

无推理后端。部署后端是 Zephyr-RTOS 嵌入式固件：

- [README.md](README.md)：使用 west 初始化、更新和安装 Zephyr SDK。
- [configs/Kconfig](configs/Kconfig)：Breeze 主配置入口。
- [configs/rm_typec.dts](configs/rm_typec.dts)、[configs/damiao_mc02.dts](configs/damiao_mc02.dts)：板级设备树。
- [configs/motor.binding.yaml](configs/motor.binding.yaml)、[configs/can_rx.binding.yaml](configs/can_rx.binding.yaml)、[configs/can_tx.binding.yaml](configs/can_tx.binding.yaml)、[configs/remote.binding.yaml](configs/remote.binding.yaml)：设备绑定。

主要依赖 CAN、UART、PWM、GPIO、Kconfig 和设备树，不涉及模型运行时。

## 5. 未知与验证缺口

- 未复制上层比赛应用，无法确认是否另有视觉协议或云台控制器。
- 没有实机日志，无法验证电机 ID、CAN 过滤器、发送频率、心跳超时和舵机限位参数。
- `breeze` 只给出驱动/框架边界；视觉侧模型、目标状态定义和通信协议需要在外部项目确认。
