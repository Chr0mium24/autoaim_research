# 归档结果

日期：2026-07-09

本轮只处理用户指定的两个仓库：

| 项目 | 输出目录 | 结果 |
| --- | --- | --- |
| `jlu_vision_26` | `docs/arch/jlu_vision_26/` | 已生成 `report.md`，并整理 `configs/`、`code/`、`models/` |
| `SHtech_auto_aim` | `docs/arch/SHtech_auto_aim/` | 已生成 `report.md`，并整理 `configs/`、`code/`、`models/` |

## 结论

- `jlu_vision_26` 是一套完整的 iceoryx + fast_tf + gtsam 自瞄框架，检测侧包含装甲板与能量机关两条模型链路。
- `SHtech_auto_aim` 是一套支持 ONNX / TensorRT / AXCL 的单体式自瞄工程，检测后接传统角点修正、PnP、跟踪和 TinyMPC 规划。
- 两个目录都已经按要求保留了架构分析所需的关键配置、源码和小体积模型文件。
