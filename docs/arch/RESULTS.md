# 架构整理结果

日期：2026-07-09

本轮已为两个指定仓库补齐中文架构报告，并同步了关键源码、配置和模型相关文件到各自输出目录。

## 已完成

1. `docs/arch/robomaster-cv/`
   - 已生成 `report.md`
   - 已同步 ROS2 视觉框架的源码、launch、参数、DeepStream 配置和轻量模型说明文件

2. `docs/arch/sp_vision_25_nonros/`
   - 已生成 `report.md`
   - 已同步非 ROS 自瞄框架的配置、任务代码、IO 层、工具层和关键模型文件

## 观察到的结构差异

- `robomaster-cv` 是 ROS2 图式工程，推理和跟踪都被拆成节点，DeepStream 是 Jetson 上的部署路径。
- `sp_vision_25_nonros` 是单体 C++ 工程，模型推理、后处理、跟踪、预测和控制都在同一套源码里完成。

## 复用判断

- `robomaster-cv` 更适合作为 ROS2 通信与部署模板。
- `sp_vision_25_nonros` 更适合作为算法闭环和控制策略模板。

