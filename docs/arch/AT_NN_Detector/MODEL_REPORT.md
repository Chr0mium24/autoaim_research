# AT_NN_Detector 模型报告

## 模型清单

- ONNX 模型：
  - [models/common/praysky_c2psa_e2e_0228_640x640.onnx](models/common/praysky_c2psa_e2e_0228_640x640.onnx)
  - [models/common/praysky_c2psa_e2e_0228_576x768.onnx](models/common/praysky_c2psa_e2e_0228_576x768.onnx)
  - [models/common/praysky_coord_noe2e_0331_640x640.onnx](models/common/praysky_coord_noe2e_0331_640x640.onnx)
  - [models/common/praysky_coord_noe2e_0331_576x768.onnx](models/common/praysky_coord_noe2e_0331_576x768.onnx)
- Axera 模型：[models/axera/praysky_coord_noe2e_qat_u16_all.axmodel](models/axera/praysky_coord_noe2e_qat_u16_all.axmodel)。
- 说明文件：[models/README.md](models/README.md)。
- 缺失但 Axera 源码引用的重要头文件：`code/config.hpp`、`core/armor_types.hpp`、`core/types.hpp` 未在本目录归档，因此 `ArmorAxeraConfig` 的实际默认值无法在本地闭合验证。

## ONNX 模型

| 文件 | 设计 | 输入 | 输出 |
| --- | --- | --- | --- |
| `praysky_c2psa_e2e_0228_640x640.onnx` | C2PSA，End-to-End | `images` fp32 `[1,3,640,640]` | `output0` fp32 `[1,30,18]` |
| `praysky_c2psa_e2e_0228_576x768.onnx` | C2PSA，End-to-End | `images` fp32 `[1,3,576,768]` | `output0` fp32 `[1,30,18]` |
| `praysky_coord_noe2e_0331_640x640.onnx` | CoordAtt，NMS 导出 | `images` fp32 `[1,3,640,640]` | `output0` fp32 `[1,30,18]` |
| `praysky_coord_noe2e_0331_576x768.onnx` | CoordAtt，NMS 导出 | `images` fp32 `[1,3,576,768]` | `output0` fp32 `[1,30,18]` |

- 任务：装甲板检测与姿态关键点输出，基于 README 描述的 Ultralytics YOLO26n-Pose 改造。
- 输出布局：每条检测为 `[x1, y1, x2, y2, conf, class_id, color_scores(4), kpts(8)]`。
- 类别：12 个装甲板类别，覆盖小/大装甲与哨兵、前哨站、基地大装甲等映射，详见 [models/README.md](models/README.md)。
- 颜色：4 类 `B/R/G/P`。
- 关键点：4 个 2D 角点，README 写明顺时针排列。
- 代码核验：[code/preview.py](code/preview.py) 对 4 个 ONNX 使用相同解析方式。

## Axera `.axmodel`

- 任务：同一装甲板关键点检测任务的 Axera650 部署版本。
- 格式：`.axmodel`，README 标注为 CoordAttn 方案，QAT+PTQ，w8a16 量化。
- 输入：本目录缺少 `ArmorAxeraConfig`，无法确认默认输入宽高；[code/axera/axera.cpp](code/axera/axera.cpp) 通过 `config_.input_width/input_height` 做 BGR888 直接 resize。
- 输出：CPU 后处理按 concat-predecode tensor 解码，维度公式为 `4 + num_colors + num_pairs + num_kpts*2`；具体 `num_*` 默认值因配置头缺失无法本地确认。
- 设计特征：模型侧保留 anchor/predecode 输出，CPU 侧做 top-k、sigmoid 阈值、颜色筛选、关键点重排和 tile-based NMS。
- 类别映射：源码内 `PAIR_TO_ARMOR` 将 12 个 pair 映射到 `Sentry/One/Two/.../Outpost/BaseLarge`。

## 预处理与后处理

- ONNX 预览：letterbox resize + `114` padding，OpenCV BGR 转 RGB，归一化到 `[0,1]`，NCHW。
- ONNX 后处理：模型已经输出最多 30 条结果，[code/preview.py](code/preview.py) 只做置信度过滤、坐标反变换、颜色 argmax 和绘制。
- Axera 预处理：优先 IVPS 硬件 resize，失败回退 OpenCV resize；源码说明非 4:3 输入会直接拉伸到模型输入尺寸。
- Axera 后处理：对象 logits 取 top-k，颜色 logits 过滤目标颜色，关键点按 `KPT_ORDER={0,3,2,1}` 重排，最后按角点外接框做 tile NMS。

## 部署后端

- ONNX Runtime：用于 [code/preview.py](code/preview.py)，优先 CUDA provider，回退 CPU provider。
- Axera NPU：通过 `AX_SYS_*`、`AX_ENGINE_*`、`AX_IVPS_*` API 完成模型加载、内存管理、硬件 resize 和同步推理。

## 未知与核验缺口

- Axera 端缺少配置和核心类型头文件，无法独立编译，也无法确认 `top_k`、`strides`、输入宽高、NMS 阈值等默认值。
- `.axmodel` 未通过真实 Axera SDK 读取 IO metadata；这里只依据 README 和源码解码逻辑。
- 未归档训练权重、数据集、导出脚本和量化报表。
- [code/preview.py](code/preview.py) 顶部旧注释曾写 e2e 输出为 14 维，但当前 README、ONNX 元数据和实际解析均为 18 维。
