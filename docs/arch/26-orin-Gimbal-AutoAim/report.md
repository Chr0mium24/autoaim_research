# 26-orin-Gimbal-AutoAim 架构报告

## 项目定位

这个仓库不是完整的视觉检测项目，而是 RoboMaster 云台侧的嵌入式控制模板。仓库核心在 `SEML/`，重点是消息总线、CAN 电机抽象、云台双环 PID 和 USB 通信接口。

从现有代码看，`Robo_AA.c/.h`、`Robo_USB.c/.h` 都还是空壳或未完成状态，`AA_Task()` 没有实现，自瞄目标解算并不在这里完成。也就是说，这个仓库更像云台执行层和工程模板，模型检测与识别很可能依赖上游 PC/NPU 侧系统。

## 模型架构

仓库内没有神经网络模型、ONNX、TensorRT、OpenVINO 或其他视觉推理代码，也没有检测器/后处理实现。

`Robo_AA.c` 目前为空实现，说明所谓“自瞄”链路的目标获取、弹道解算和目标点发布还没有在该仓库中落地。这里能确认的是：云台控制会消费上游通过消息总线发布的角度目标，而不是自己直接做视觉推理。

## 输入输出

`SEML/App/Robo/Robo_gimbal.c` 中的云台控制输入主要是：

- `Set_Gimbal_Yaw_Angle`
- `Set_Gimbal_Pitch_Angle`
- `Set_Gimbal_Imu_Data`

输出是对 Yaw/Pitch 两个 GM6020 电机的电流控制，最终通过 `Motor_Send_Data()` 下发。

`Robo_Common.c/.h` 里初始化了 CAN 总线、AHRS 姿态解算和消息总线，是整个闭环的数据底座。

## 推理后端

没有独立推理后端。

如果要把这个工程接到视觉系统，推理通常应当在上位机、Jetson、昇腾或其他外部平台完成，然后把目标角度通过消息总线或串口/USB 传给这里。

## 后处理 / 跟踪 / 预测

仓库自身没有检测后处理、目标跟踪或运动预测模块。

已存在的控制相关逻辑是：

- `Robo_gimbal.c`：云台双环 PID，yaw/pitch 位置环 + 速度环串级控制。
- `pid.c/.h`：通用 PID 计算器。
- `Dji_Motor.c/.h`：大疆电机封装、CAN 报文收发、看门狗超时处理。

这意味着仓库的“预测”只体现在云台控制环内，没有视觉层的轨迹预测。

## 关键配置

已复制的关键配置文件：

- `configs/RobomasterBoardTypeC.ioc`：STM32CubeMX 工程配置。
- `configs/MDK-ARM.uvprojx`：Keil 工程文件。
- `configs/MDK-ARM.uvoptx`：Keil 工程选项。
- `configs/SEML-build-CMakeLists.txt`：SEML 构建入口。

这些配置说明项目目标平台是 STM32F407 类 RoboMaster 板卡，工程重点在外设、CAN、USB 和电机驱动。

## 关键代码

已复制的关键源码：

- `code/SEML/App/Robo/Robo_gimbal.c`
- `code/SEML/App/Robo/Robo_gimbal.h`
- `code/SEML/App/Robo/Robo_AA.c`
- `code/SEML/App/Robo/Robo_AA.h`
- `code/SEML/App/Robo/Robo_USB.c`
- `code/SEML/App/Robo/Robo_USB.h`
- `code/SEML/App/Robo/Robo_Common.h`
- `code/SEML/App/Robo/Robo_common.c`
- `code/SEML/App/Robo/pid.c`
- `code/SEML/App/Robo/pid.h`
- `code/SEML/Middlewares/Communications/Dji_Motor.c`
- `code/SEML/Middlewares/Communications/Dji_Motor.h`
- `code/SEML/Drivers/hal/Interface/can_if.c`
- `code/SEML/Drivers/hal/Interface/can_if.h`

关键观察：

- `Robo_gimbal.c` 已经把电机绑定、PID 初始化和闭环控制串起来。
- `Robo_AA.c` 仍为空，云台自瞄决策没有在本仓库实现。
- `Robo_USB.c` 的 `send/receive/init_packet` 都未实现，PC/上位机链路还在模板状态。
- `Dji_Motor.c` 里有不少 `TODO`，例如发送 ID 分配、CAN 发送和速度环初始化还没补完。

## 可复用建议

适合直接复用的部分：

- `Robo_Common` 的消息总线和 AHRS 初始化思路。
- `Robo_gimbal` 的双环 PID 云台控制框架。
- `Dji_Motor` 的电机抽象接口和看门狗机制。

需要适配的部分：

- 话题名和消息结构要和你的上位机协议统一。
- `PID` 参数、云台限速和电机型号要按机械平台重新标定。
- `USB` 收发包结构需要按实战协议补齐。

仅作参考的部分：

- `AA_Task()` 空壳和 `Robo_USB.c` 的模板实现。
- `Dji_Motor.c` 中未完成的 `TODO` 段。

## 结论

这是一个偏执行层的 RoboMaster 云台/控制工程，不是独立视觉模型仓库。模型和检测链路缺失，当前可复用价值主要集中在云台闭环、电机抽象和消息总线设计。
