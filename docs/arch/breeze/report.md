# breeze 架构报告

## 项目定位

`breeze` 是深圳大学 RobotPilots 战队的 Zephyr-RTOS 嵌入式开发框架。

它不是视觉自瞄工程，也没有看到训练、推理、检测器或相机管线代码。它的职责是：

- 接收遥控器输入。
- 管理 CAN 总线收发。
- 适配 DJI 电机和 PWM 舵机。
- 提供板级移植、驱动抽象和样例程序。

如果上层是视觉自瞄系统，breeze 更像“执行端”和“设备层”，不是“识别端”。

## 是否包含视觉/自瞄模型代码

结论：不包含。

仓库里没有 OpenCV / DNN / ONNX / TensorRT / 检测模型相关代码；没有相机读取、目标检测、PnP 解算或后处理逻辑。

与自瞄相关的内容主要是控制接口：

- 遥控器输入 `remote`
- CAN 收发 `can_rx_manager` / `can_tx_manager`
- DJI 电机驱动 `motor/dji`
- PWM 舵机 `servo_motor_pwm`

## 与视觉自瞄的接口边界

这个仓库把边界收在控制层：

- 外部视觉系统负责给出 pitch / yaw / 距离 / 目标状态。
- breeze 负责把上层控制量转成电机 CAN 电流、舵机 PWM、遥控输入和板级 I/O。

仓库本身没有定义“视觉消息协议”，因此视觉侧如果要接入，通常需要在应用层再定义一层串口、CAN 或共享内存协议。

## 架构组成

### 1. 遥控输入

`drivers/remote/dr16_remote.c` 负责解析 DJI DR16/DT7 遥控器。

它支持：

- 4 路摇杆通道
- 两个三段开关
- 旋钮/拨轮
- 鼠标速度与按键
- 长按/短按状态机
- 心跳与离线检测

`include/drivers/remote.h` 暴露了 `remote_get_sensor()` 和 `remote_set_data_ready_cb()`，适合作为上层控制入口。

### 2. CAN 收发管理

`drivers/can_tx_manager/can_tx_manager.c` 和 `drivers/can_rx_manager/can_rx_manager.c` 把 Zephyr 原生 CAN 封装成软件级发送/接收管理器：

- TX manager 支持注册多个发送者。
- 支持事件触发和定频发送。
- RX manager 负责软件过滤和回调分发。
- 代码还维护了总线负载统计。

这层适合承接电机、云台、射击机构和其他 CAN 设备。

### 3. DJI 电机驱动

`drivers/motor/dji/can_dji.c` 是最关键的控制代码。

它做了三件事：

1. 注册电机到 RX manager，解析反馈帧中的编码器、速度、电流和温度。
2. 注册电机到 TX manager，把目标电流序列化到 CAN 帧里。
3. 提供心跳检测，超时后把电机视为离线。

`include/drivers/motor.h` 定义了统一电机 API：

- `register_motor`
- `get_motor_rxdata`
- `motor_torque_control`
- `motor_change_tx_feq`
- `get_motor_heartbeat_status`

这就是外部应用应该调用的接口边界。

### 4. PWM 舵机

`drivers/servo_motor_pwm/servo_motor_pwm.c` 和 `include/drivers/servo_motor_pwm.h` 提供舵机角度、偏置和机械限位接口。

这部分更接近云台角度执行器，而不是视觉算法。

## 推理与部署

无视觉推理后端。

这是纯嵌入式控制框架，运行在 Zephyr-RTOS 上，依赖：

- CAN
- UART
- PWM
- GPIO
- I2C / SPI
- Zephyr 设备树和 Kconfig

## 关键配置

已复制到 `docs/arch/breeze/configs/` 的关键配置：

- `Kconfig`：Breeze 主开关及内核能力开关。
- `drivers/Kconfig`：驱动子模块装配入口。
- `drivers/motor/Kconfig`：电机驱动、心跳检测参数。
- `drivers/remote/Kconfig` 与 `Kconfig.DR16`：遥控器支持。
- `drivers/can_tx_manager/Kconfig`、`drivers/can_rx_manager/Kconfig`：CAN 管理器配置。
- `drivers/servo_motor_pwm/Kconfig`：PWM 舵机配置。
- `boards/dji/rm_typec/*.dts|*.defconfig|*.yaml`：DJI Type-C 板级支持。
- `boards/damiao/mc02/*.dts|*.defconfig|board.yml`：达妙 MC02 板级支持。
- `dts/bindings/*`：遥控、CAN、DJI 电机、舵机绑定定义。

其中 `rm_typec.dts` 和 `damiao_mc02.dts` 明确暴露了 CAN、USART、PWM、遥控器和传感器资源，是上层控制系统最重要的板级入口。

## 关键代码

已复制到 `docs/arch/breeze/code/` 的关键源码：

- `include/drivers/remote.h` / `drivers/remote/dr16_remote.c`
- `include/drivers/can_tx_manager.h` / `drivers/can_tx_manager/can_tx_manager.c`
- `include/drivers/can_rx_manager.h` / `drivers/can_rx_manager/can_rx_manager.c`
- `include/drivers/motor.h` / `drivers/motor/dji/can_dji.c`
- `include/drivers/lk_motor.h` / `drivers/motor/dji/dji_protocol.h`
- `include/drivers/servo_motor_pwm.h` / `drivers/servo_motor_pwm/servo_motor_pwm.c`
- `samples/boards/dm_mc02/remote/src/main.c`
- `samples/boards/dm_mc02/can_tx_manage/src/main.c`
- `samples/boards/dm_mc02/can_rx_manage/src/main.c`
- `samples/boards/dm_mc02/motor/src/main.cpp`

样例程序说明了各 API 的调用方式：

- 遥控样例打印通道、拨轮和按键状态。
- CAN TX 样例演示事件触发和周期发送。
- CAN RX 样例演示过滤器回调。
- 电机样例演示 `register_motor()`、`get_motor_rxdata()` 和 `motor_update_serialized()` 的控制闭环。

## 可复用建议

可直接复用的部分：

- DJI 电机的 CAN RX/TX 封装。
- CAN manager 的软件分发模式。
- 遥控器输入解析与离线检测。
- PWM 舵机的角度抽象。
- 板级 `dts/defconfig` 组织方式。

需要适配的部分：

- 视觉自瞄侧的消息协议需要另行定义。
- 电机 ID、CAN 过滤器、板级引脚和时钟配置要按实际硬件修改。
- 心跳超时、发送频率和控制量范围需要结合上层控制策略重新标定。

仅作参考的部分：

- 样例程序主要用于验证驱动，不是完整比赛策略。
- 部分板级定义和传感器组合是针对特定硬件的。

