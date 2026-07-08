# 自瞄视觉架构归档结果

日期：2026-07-09

## 交付范围

本轮使用多个 `gpt-5.4-mini` subagent 并行分析 `open_source_vision_repos/` 下已经下载的视觉、自瞄、检测模型和相关框架仓库。每个项目一个目录，目录内包含中文 `report.md`，并复制出与模型架构、推理、后处理、跟踪预测、相机/部署或自瞄接口相关的关键配置和源码。

`sentry-auto-aim` 初次因当前环境无法从 Gitee clone 到源码而跳过；源码后续补入工作区后，已追加完成源码级报告。

## 项目结果

| 项目 | 输出目录 | 架构结论 | 归档内容 |
| --- | --- | --- | --- |
| `jlu_vision_26` | `docs/arch/jlu_vision_26/` | 完整自瞄/能量机关框架，装甲板侧为 YOLOv5 ONNX + OpenVINO，能量机关侧为 YOLOX ONNX；后端包含 iceoryx 通信、fast_tf、gtsam/因子图式跟踪预测。 | `configs/`、`code/`、`models/` |
| `SHtech_auto_aim` | `docs/arch/SHtech_auto_aim/` | 单体式自瞄工程，支持 ONNX/TensorRT/AXCL/AX650 相关部署；检测后接角点优化、PnP、跟踪预测和 TinyMPC 规划。 | `configs/`、`code/`、`models/` |
| `Climber_Vision_26` | `docs/arch/Climber_Vision_26/` | 基于 `sp_vision_25_nonros` 深改的完整视觉框架，装甲板支持 YOLOv5/YOLOv8/YOLO11 OpenVINO XML，能量机关含 RepVGG/YOLO11 相关模型，后端接 EKF 和目标选择。 | `configs/`、`code/`、`models/` |
| `TGU_Vision_2026` | `docs/arch/TGU_Vision_2026/` | RM2026 非 ROS 自瞄框架，结构与 sp_vision 类似，装甲板支持多 YOLO 版本，能量机关使用 YOLO11/OpenVINO，含多线程检测、EKF 跟踪和 MPC/规划布局。 | `configs/`、`code/`、`models/` |
| `AT_NN_Detector` | `docs/arch/AT_NN_Detector/` | 检测模型包，提供多种 ONNX 输入尺寸和 Axera `axmodel`；代码侧包含 ONNX 预览和 AX650 raw decode + tile NMS。 | `configs/`、`code/`、`models/` |
| `RobotDetectionModel` | `docs/arch/RobotDetectionModel/` | 深圳大学 RobotPilots 检测模型，OpenVINO C++ 推理封装，固定 640 输入，后处理包含颜色过滤、关键点构框和 NMS；不包含跟踪预测闭环。 | `configs/`、`code/`、`models/` |
| `26-orin-Gimbal-AutoAim` | `docs/arch/26-orin-Gimbal-AutoAim/` | 偏 STM32/SEML 云台控制与自瞄跟随接口工程，没有独立视觉模型；重点是 Orin/上位机结果到云台控制、PID、USB/CAN/DJI 电机链路。 | `configs/`、`code/` |
| `AIRS-RM-2025` | `docs/arch/AIRS-RM-2025/` | Python 自瞄闭环，使用 Ascend `ais_bench` 加载 `.om` 模型，后续接检测解析、Kalman 跟踪、目标评分、测距和串口/云台控制。 | `configs/`、`code/`、`models/` |
| `robomaster-cv` | `docs/arch/robomaster-cv/` | ROS2 节点化视觉框架，训练/测试依赖 Darknet，Jetson 部署依赖 DeepStream-Yolo/TensorRT；项目自身通过 YAML/TXT 参数组织相机、YOLO、DeepStream 和串口节点。 | `configs/`、`code/`、`models/`、`doc/` |
| `sp_vision_25_nonros` | `docs/arch/sp_vision_25_nonros/` | 同济非 ROS 视觉框架基线，OpenVINO + YOLOv5/v8/v11 装甲板检测，YOLO11 能量机关检测，后端接 EKF、弹道解算、MPC/控制接口。 | `configs/`、`code/`、`models/`、`assets/` |
| `sentry-auto-aim` | `docs/arch/sentry-auto-aim/` | 辽宁科技大学 COD ROS2 哨兵自瞄/能量机关框架；装甲板为传统灯条几何检测 + ONNX LeNet/MLP 数字分类 + PCA/BA/PnP，能量机关为 YOLOX 关键点模型 + OpenVINO，后端接 EKF、选板、弹道补偿和串口控制。 | `configs/`、`code/`、`models/`、`interfaces/` |
| `RP_Infantry_Plus` | `docs/arch/RP_Infantry_Plus/` | 深圳大学旧步兵视觉工程，装甲板主链路为颜色/轮廓/几何筛选的传统 CV；能量机关部分含 tiny-YOLOv2/LeNet Caffe 资源。 | `configs/`、`code/`、`models/` |
| `breeze` | `docs/arch/breeze/` | Zephyr 嵌入式控制框架，不含视觉或神经网络模型；可作为自瞄系统下位机 CAN、遥控、电机、舵机和板级配置参考。 | `configs/`、`code/`、`models/README.md` |

## 复核结果

- 已生成 13 个项目目录和 13 份中文 `report.md`。
- 每个项目目录均包含复制出的相关配置、源码或模型说明；确实不含视觉模型的项目在报告中明确说明边界。
- 外部源码仓库保留在 `open_source_vision_repos/`，该目录作为下载缓存继续被 `.gitignore` 忽略。
- 未复制超过 50 MB 的单个模型/权重文件；已复制的二进制模型主要是小体积 ONNX、AXMODEL 或 OpenVINO XML 配套资源。
- 当前总归档入口为 `docs/arch/PLAN.md` 和 `docs/arch/RESULTS.md`。
