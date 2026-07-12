# 最新视觉模型仓库调研计划

日期：2026-07-12

## 目标

整理当前归档中最新的几个 RoboMaster 视觉/自瞄模型相关仓库，形成一份中文调研文档，重点回答：

1. 每个仓库有哪些模型文件。
2. 模型结构和输出属性是什么。
3. 运行模型的后端架构是什么。
4. 能运行在哪些设备或平台上。
5. 哪些信息已经由本地文件确认，哪些仍需要实机或模型解析验证。

## 选取范围

按 2026-07-12 重新查询的远端配置分支最新 commit 日期，选取最新的视觉/自瞄模型相关仓库：

| 仓库 | 分支 | 最新 commit 日期 | 说明 |
| --- | --- | --- | --- |
| `Climber_Vision_26` | `main` | 2026-06-13 | 完整 OpenVINO 自瞄/打符框架 |
| `AT_NN_Detector` | `main` | 2026-06-11 | 装甲板关键点检测模型包 |
| `RobotDetectionModel` | `main` | 2026-06-02 | OpenVINO 装甲板检测模型 |
| `jlu_vision_26` | `master` | 2026-05-29 | OpenVINO 自瞄/能量机关框架 |
| `SHtech_auto_aim` | `ax650-dev-2026` | 2026-05-03 | 跨 AXCL/TensorRT/ONNX 后端自瞄工程 |

`breeze` 的 commit 更新到 2026-06-22，但它是嵌入式控制框架，不包含视觉模型资产，因此不作为本次模型调研主体。

## 信息来源

- 各项目 `docs/arch/<repo>/MODEL_REPORT.md`
- 各项目 `docs/arch/<repo>/report.md`
- 本地归档的 `models/`、`configs/` 和关键 `code/`
- 远端 GitHub API 查询的最新 commit 元数据

## 输出

- 完整调研正文：`docs/arch/LATEST_MODEL_RESEARCH_2026-07-12.md`
- 调研结果记录：`docs/arch/LATEST_MODEL_RESEARCH_RESULTS_2026-07-12.md`
