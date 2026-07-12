# 26-orin-Gimbal-AutoAim 模型报告

## 1. 模型清单

本目录未发现神经网络模型资产，也未发现 ONNX、TensorRT、OpenVINO、Darknet、Caffe 或 OpenCV DNN 推理入口。

已检查的本地材料包括 [report.md](report.md)、[README.md](README.md)、`configs/` 与 `code/SEML/` 下的云台、USB、自瞄和 CAN 相关代码。`code/SEML/App/Robo/Robo_AA.c` 中 `AA_Init()`、`AA_Task()` 为空实现，`code/SEML/App/Robo/Robo_USB.c` 中 `send()`、`receive()`、`init_packet()` 也为空实现。

## 2. 模型细节

无可报告的模型格式、输入尺寸、输出布局、类别或关键点定义。

README 将 AA 进程描述为“使用目标位置进行弹道解算，然后发布云台预期角度”，但本地代码没有给出目标检测模型、弹道解算实现或视觉数据包字段。

## 3. 预处理、后处理与控制逻辑

本项目可确认的是执行层控制逻辑，而不是视觉模型逻辑：

- [Robo_gimbal.c](code/SEML/App/Robo/Robo_gimbal.c)：从消息总线读取 `Set_Gimbal_Yaw_Angle`、`Set_Gimbal_Pitch_Angle`、`Set_Gimbal_Imu_Data`，对 yaw/pitch 做角度环和速度环串级 PID，输出 GM6020 电机电流。
- [Robo_common.c](code/SEML/App/Robo/Robo_common.c)：初始化消息总线、CAN1/CAN2、AHRS/Mahony 姿态解算和定时器。
- [Robo_USB.c](code/SEML/App/Robo/Robo_USB.c)：只初始化 USB CDC，收发包逻辑未完成。
- [Robo_AA.h](code/SEML/App/Robo/Robo_AA.h)：声明了 PC 收发包、期望 yaw/pitch 增量和 PID 指针，但没有实现数据更新。

因此这里没有图像预处理、检测后处理、NMS、目标跟踪或视觉预测；只有云台闭环、CAN 电机发送和 IMU 姿态反馈。

## 4. 部署后端

无推理后端。项目部署面向 STM32/RoboMaster C 板一类嵌入式控制环境：

- [RobomasterBoardTypeC.ioc](configs/RobomasterBoardTypeC.ioc)：配置 CAN、USB CDC、UART、定时器等外设。
- [MDK-ARM.uvprojx](configs/MDK-ARM.uvprojx)：Keil 工程。
- [SEML-build-CMakeLists.txt](configs/SEML-build-CMakeLists.txt)：SEML 构建入口。

如需接入视觉模型，推理应在外部 PC、Jetson/Orin 或其他上位机完成，再通过 USB/CAN/消息总线把目标角度或目标状态传给本工程。

## 5. 未知与验证缺口

- 本地没有视觉上位机代码，无法确认 Orin 侧是否另有模型。
- USB 协议结构体只有 `cmd` 和 `checksum`，无法确认实际视觉数据字段。
- `AA_Task()` 未实现，弹道模型、空气阻力补偿和速度限制仅出现在注释历史里，无法验证。
- PID 参数和电机 ID 能从代码看到，但未结合实机标定验证。
