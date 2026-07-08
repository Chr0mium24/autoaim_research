# RP_Infantry_Plus 架构报告

## 项目定位

`RP_Infantry_Plus` 是深圳大学步兵视觉开源方案，核心目标是为 RoboMaster 步兵提供装甲板识别、大符识别、PnP 角度解算和串口下发控制量。

它不是纯神经网络工程，而是以传统视觉为主：

- 装甲板识别：颜色分割、阈值二值化、轮廓提取、旋转矩形筛选、灯条配对。
- 大小符识别：可选 YOLO 先做 ROI，再做二值化、轮廓筛选、可选 LeNet 分类和圆周预测。
- 输出：通过自定义串口协议把 pitch / yaw / distance / 状态标志发给下位机。

## 模型架构

### 1. 装甲板链路

主链路在 `src/Aim/ArmorDetector.cpp` 和 `src/ImageConsProd.cpp`。

流程是典型传统 CV：

1. 使用上一帧结果做 ROI，丢失帧数累积后逐步扩大搜索范围。
2. 对图像做灰度阈值或颜色通道处理，提取红/蓝灯条。
3. 用 `findContours` 找轮廓，取 `minAreaRect` 得到旋转矩形。
4. 用灯条高度、宽高比、左右间距、上下偏差、角度差等规则筛选。
5. 两两配对成候选装甲板，再按几何约束挑出最优目标。
6. 进入 `AngleSolver` 做 PnP 解算，得到 yaw / pitch / 距离。

这条链路没有端到端检测网络，核心是规则、形状和 PnP。

### 2. 大小符链路

大小符在 `include/Rune/Detect.h` 和 `src/Rune/Detect.cpp` 中实现，属于“传统视觉 + 可选轻量网络”的混合方案：

- `sParam.use_yolo = 1` 时，使用 Darknet YOLO 先检测扇叶 ROI。
- `sParam.use_yolo = 0` 时，直接根据上一帧 `lastData` 走 ROI。
- `sParam.use_lenet = 1` 时，使用 LeNet 区分未打过的扇叶。
- 之后仍然回到轮廓、矩形、面积、长宽比、矩匹配、圆周预测等传统方法。

因此它不是单一 CNN 检测器，而是“网络辅助的传统视觉流水线”。

## 推理与部署

这里的“推理后端”主要有两类：

- OpenCV DNN `readNetFromDarknet`：用于 YOLO。
- OpenCV DNN `readNetFromCaffe`：用于 LeNet 分类。

没有看到 ONNX / TensorRT / OpenVINO / DeepStream 之类的部署入口。

相机标定文件通过 OpenCV `FileStorage` 读取，PnP 解算则用 OpenCV `calib3d`。

## 输入输出

### 输入

- 工业相机帧。
- `extraFile/AimXMl/calib_no_4_1280.yml` 中的相机内参和畸变参数。
- `extraFile/AimXMl/param_config.yml` 中的装甲板阈值、曝光、饱和度、内参文件路径等配置。
- 串口下位机模式字节：红/蓝、自瞄、哨兵模式、基地模式等。

### 输出

- `yaw_angle`
- `pitch_angle`
- `distance`
- 识别状态位：是否找到目标、是否进入可开火区、是否识别大符、是否贴脸等

输出通过 `SerialPort::TransformData()` 打包到串口。

## 后处理 / 跟踪 / 预测

### 装甲板

- ROI 跟踪依赖上一帧 `last_result`。
- 丢失计数 `_lost_cnt` 用于扩大搜索区域。
- `AngleSolver` 做 PnP 后处理。
- 距离在主循环里还做了经验拟合修正，而不是只依赖 PnP 距离。

### 大小符

- `Detect::predict()` 提供三种预测方式：
  - `FIT_CIRCLE`
  - `PUSH_CIRCLE`
  - `TANGENT`
- `isCut()` 用于判断扇叶是否切换。
- 代码里对哨兵模式单独做了丢帧保护，避免目标短暂丢失后云台停顿。

## 关键配置

已复制到 `docs/arch/RP_Infantry_Plus/configs/` 的关键配置：

- `param_config.yml`：装甲板阈值、曝光、饱和度、内参文件路径等主配置。
- `calib_no_4_1280.yml`：相机标定文件。
- `Mono_out.xml`：Rune 链路使用的相机标定/导出文件。

`include/header.h` 里还硬编码了当前工程的模型路径，说明原始工程偏“本地绝对路径 + 手动部署”风格。

## 关键代码

已复制到 `docs/arch/RP_Infantry_Plus/code/` 的关键源码：

- `main.cpp`：启动串口、读取配置、创建图像生产/消费线程。
- `include/ImageConsProd.hpp` / `src/ImageConsProd.cpp`：整条视觉主循环，负责相机读帧、模式分发、检测、解算、串口发送。
- `include/Aim/ArmorDetector.hpp` / `src/Aim/ArmorDetector.cpp`：装甲板传统视觉检测。
- `include/Aim/AngleSolver.hpp` / `src/Aim/AngleSolver.cpp`：PnP 解算。
- `include/Rune/Detect.h` / `src/Rune/Detect.cpp`：大小符检测、预测、切换判断。
- `include/SerialPort/serialport.h` / `src/SerialPort/serialport.cpp`：串口协议和下位机通信。
- `include/SerialPort/CRC_Check.h` / `src/SerialPort/CRC_Check.cpp`：CRC8 / CRC16 校验。
- `include/Camera/video.h` / `src/Camera/video.cpp`：相机 SDK 封装。

## 可复用建议

可直接复用的部分：

- 灯条筛选和装甲板几何匹配逻辑。
- ROI 收缩与丢失帧逐步扩张的策略。
- `AngleSolver` 的 PnP 解算接口。
- 大符的圆周预测和切换判定思路。
- 串口 CRC 打包与状态位设计。

需要适配的部分：

- 大华相机 SDK 封装和绝对路径配置。
- 具体阈值、曝光和相机内参。
- 目标尺寸、哨兵模式、基地模式的业务逻辑。

仅作参考的部分：

- 代码里部分经验拟合距离公式和固定阈值，强依赖原赛季相机和场地条件。

## 额外说明

README 描述的串口协议是 30 字节，但 `serialport.cpp` 中实际发送的是 22 字节帧；复用时应以代码实现为准，并重新核对协议长度、CRC 和字段布局。

