# jlu_vision_26 架构报告

## 项目定位

`jlu_vision_26` 是一套完整的 RoboMaster 自瞄与能量机关视觉框架，特点是不依赖 ROS，使用 iceoryx 做跨进程零拷贝通信，使用 fast_tf 管理坐标系，跟踪侧引入了基于重投影误差的因子图优化。

它同时覆盖：

- 装甲板自瞄
- 能量机关识别与跟踪
- 相机、串口、IMU、坐标系广播
- 状态反馈可视化

## 模型架构

### 装甲板检测

检测模型使用 `assets/0526.onnx`，对应装甲板四角点检测网络。代码里把它当作 YOLO 风格的角点模型来处理，输出包含：

- 4 个角点坐标
- 目标置信度
- 装甲板类别
- 敌方颜色

输入在 `armor_detector` 里被缩放到固定尺寸后送入推理，随后做：

- 置信度筛选
- NMS
- 角点顺序修正
- 角点坐标映射回原图
- PCA 灯条矫正
- PnP 解算

其中 `LightCornerCorrector` 负责传统视觉修正，`PnPSolver` 负责位姿恢复。

### 能量机关检测

能量机关扇叶检测使用 `assets/yolox_rune_3.6m.onnx`，是另一套 YOLOX 风格网络。输入固定为 480x480，输出每个候选扇叶的：

- 5 个关键点
- 类别
- 颜色
- 置信度

后处理会做：

- 候选生成
- NMS / 合并
- 关键点加权平均
- 扇叶中心与轮廓重建

## 推理后端

这个仓库的推理后端主要由 OpenVINO 承担，代码在 `src/auto_aim/*/yolo.hpp` 和 `src/auto_buff/*/yolo.hpp` 中统一封装为 `preProcess / requestInfer / postProcess` 形式。

从代码和配置看，它支持面向低时延和吞吐两种模式的 OpenVINO 推理调用，但项目整体定位仍然是 CPU / OpenVINO 侧的通用部署，而不是强绑定某一张卡。

## 后处理、跟踪与预测

### 装甲板跟踪

`armor_tracker` 不是简单的卡尔曼滤波，而是把装甲板和整车状态分成多层状态机来处理：

- 单块装甲板状态
- 整车状态
- 哨兵 / 前哨等特殊目标

它的关键特点是使用重投影误差约束的因子图优化来抑制抖动，报告里可以概括成“PNP 初值 + 因子图平滑 + 速度一致性约束”的组合。

### 弹道规划

`planner` 负责把跟踪状态转成云台控制命令，核心动作包括：

- 预测子弹飞行时间
- 叠加算法延迟与串口延迟
- 生成目标 yaw / pitch
- 计算开火阈值
- 对移动目标做提前量补偿

它把静态目标、装甲板模型、整车模型分成不同策略，按目标速度和状态切换。

### 能量机关跟踪

`buff_tracker` 分为小符和大符两套逻辑：

- 小符更偏因子图和角速度跟踪
- 大符使用 `BuffFitter` 做曲线拟合，再结合弹道轨迹求解

这部分比装甲板跟踪更强调周期运动预测。

## 关键配置

已复制的关键配置主要有：

- `configs/auto_aim/armor_detector.yaml`：装甲板检测、推理后端、PCA 和 PnP 参数
- `configs/auto_aim/armor_tracker.yaml`：装甲板跟踪、NIS 检查、因子图噪声、弹道规划参数
- `configs/auto_buff/buff_detector.yaml`：扇叶检测模型、阈值和传统纠正参数
- `configs/auto_buff/buff_tracker.yaml`：小符 / 大符跟踪、拟合与弹道参数
- `configs/hardware/camera.yaml`、`imu.yaml`、`serial.yaml`：硬件输入输出参数
- `configs/odom_coord/*.yaml`：静态坐标变换与 transform 聚合
- `configs/startup/*.yaml`：启动编排、模块组合、调试模式
- `configs/status_feedback/*.yaml`：可视化反馈配置
- `configs/iox-roudi_config.toml`：iceoryx 运行时配置

## 关键代码

已复制的关键源码路径：

- `code/src/auto_aim/armor_detector/*`：装甲板检测、灯条修正、PnP 解算
- `code/src/auto_aim/armor_tracker/*`：装甲板目标跟踪、选板、弹道规划
- `code/src/auto_buff/buff_detector/*`：扇叶检测与中心修正
- `code/src/auto_buff/buff_tracker/*`：小符 / 大符跟踪与拟合
- `code/src/common_defs/*`：消息、坐标与公共类型
- `code/src/hardware/*`：相机、串口、IMU 接入
- `code/src/odom_coord/*`：坐标系广播与 transform 聚合
- `code/src/status_feedback/*`：Open3D 可视化
- `code/startup/*`：多进程启动脚本

## 可复用建议

这套工程最值得复用的部分有三块：

1. 检测器输出到 PnP 的链路很完整，适合直接搬到别的自瞄项目里做“角点模型 + 传统几何”方案。
2. 装甲板跟踪中的因子图思路很强，尤其适合想减少识别抖动、提升状态平滑性的项目。
3. 能量机关的 `YOLO + 传统纠正 + 拟合预测` 结构，也适合迁移到其他周期运动目标。

需要适配的地方也很明确：

- 模型输出格式和关键点顺序
- iceoryx 话题名
- 相机标定与坐标系定义
- 各类噪声参数和弹道参数

## 本次归档说明

本目录已整理出：

- `configs/`
- `code/`
- `models/`

其中 `models/` 只保留了较小的 ONNX 权重文件，不包含视频和构建产物。

