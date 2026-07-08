# 自瞄视觉模型架构分析计划

日期：2026-07-09

## 目标

分析 `open_source_vision_repos/` 下已下载的 RoboMaster 自瞄、视觉框架、检测模型相关仓库，梳理每个项目的模型架构、推理入口、配置组织和可复用代码位置。

每个项目在 `docs/arch/<项目名>/` 下单独建目录，目录内至少包含：

- `report.md`：中文架构报告。
- `configs/`：复制出的模型、推理、相机、装甲板、追踪或部署相关配置文件。
- `code/`：复制出的模型定义、检测器、推理封装、后处理、跟踪/预测等关键源码。
- `models/`：仅复制轻量的模型说明、label、yaml、导出脚本等；不强制复制大体积权重。

## 仓库范围

| 项目 | 源码路径 | 备注 |
| --- | --- | --- |
| `jlu_vision_26` | `open_source_vision_repos/jlu_vision_26` | 完整自瞄视觉框架 |
| `SHtech_auto_aim` | `open_source_vision_repos/SHtech_auto_aim` | AX650/NX 自瞄视觉项目 |
| `Climber_Vision_26` | `open_source_vision_repos/Climber_Vision_26` | 17mm 自瞄视觉框架 |
| `TGU_Vision_2026` | `open_source_vision_repos/TGU_Vision_2026` | RM2026 自瞄框架 |
| `AT_NN_Detector` | `open_source_vision_repos/AT_NN_Detector` | 装甲板检测模型包 |
| `26-orin-Gimbal-AutoAim` | `open_source_vision_repos/26-orin-Gimbal-AutoAim` | 云台/自瞄跟随相关工程 |
| `robomaster-cv` | `open_source_vision_repos/robomaster-cv` | ROS2/DeepStream/Darknet 视觉框架 |
| `AIRS-RM-2025` | `open_source_vision_repos/AIRS-RM-2025` | Atlas/EP 平台自瞄闭环 |
| `RobotDetectionModel` | `open_source_vision_repos/RobotDetectionModel` | 深圳大学 RobotPilots 检测模型 |
| `RP_Infantry_Plus` | `open_source_vision_repos/RP_Infantry_Plus` | 深圳大学旧步兵视觉工程 |
| `sp_vision_25_nonros` | `open_source_vision_repos/sp_vision_25_nonros` | 同济非 ROS 自瞄框架基线 |
| `breeze` | `open_source_vision_repos/breeze` | 深圳大学嵌入式框架，按是否含自瞄/视觉接口说明 |

`sentry-auto-aim` 未下载成功，本轮不生成源码级架构报告。

## 报告要求

每份 `report.md` 使用中文，并覆盖：

1. 项目定位：完整自瞄、检测模型包、视觉框架、控制接口或非视觉工程。
2. 模型架构：检测网络类型、输入输出、后处理、分类/关键点/装甲板解码方式；如果没有神经网络，说明传统视觉或控制逻辑。
3. 推理与部署：ONNX/OpenVINO/TensorRT/AXRuntime/DeepStream/Darknet/Ascend 等入口。
4. 配置结构：复制出来的关键配置文件及其含义。
5. 关键代码：复制出来的源码文件及其作用。
6. 可复用建议：对当前自瞄项目可直接复用、需适配、仅作参考的部分。

## 执行方式

使用多个 `gpt-5.4-mini` subagent 并行处理，每个 agent 只负责自己分配的项目目录，避免写入冲突。

主 agent 最后统一复核：

- 每个项目是否有 `report.md`。
- 是否至少复制了若干相关配置或代码；若项目确实没有视觉/模型代码，在报告中说明。
- 是否生成 `docs/arch/RESULTS.md` 总结。

