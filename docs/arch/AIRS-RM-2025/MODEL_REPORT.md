# AIRS-RM-2025 模型报告

## 模型清单

- 已归档模型相关文件：[models/labels.txt](models/labels.txt)。
- 代码引用但本目录缺失的关键模型：`16.om`。引用位置见 [code/config.py](code/config.py)，README 也要求将 `16.om` 放在项目根目录。
- 相关推理与后处理代码：[code/autoaim.py](code/autoaim.py)、[code/detector.py](code/detector.py)、[code/det_utils.py](code/det_utils.py)、[code/utils.py](code/utils.py)。

## `16.om` 检测模型

- 任务：RoboMaster 装甲板/云台部件检测。
- 格式：华为昇腾离线 `.om` 模型；本地未归档模型本体，无法读取模型元数据。
- 后端：`ais_bench.infer.interface.InferSession(0, model_path)`。
- 输入：代码配置为 `640x640`；预热张量为 `float32`、`NCHW`、`[1, 3, 640, 640]`。
- 输入布局：摄像头帧是 OpenCV BGR；[code/utils.py](code/utils.py) 中 `preprocess_image()` 做 letterbox、`BGR -> RGB`、`HWC -> CHW`、`float32`，随后在 [code/detector.py](code/detector.py) 中除以 `255.0`。
- 输出：源码把 `raw_model_outputs[0]` 转为 Torch tensor 后送入 YOLO 风格 NMS；[code/det_utils.py](code/det_utils.py) 的 `non_max_suppression()` 期望候选格式近似 `[x, y, w, h, obj_conf, class_scores...]`，最终输出 `[xyxy, conf, cls]`。原始输出张量形状因 `16.om` 缺失无法确认。
- 类别：[models/labels.txt](models/labels.txt) 定义 4 类：`RedArmor`、`RedGimbal`、`BlueArmor`、`BlueGimbal`。
- 自瞄使用类别：[code/config.py](code/config.py) 中 `AUTO_AIM_CLASSES={0, 2}`，代码过滤掉 `1`、`3` 两类云台目标。

## 预处理与后处理

- 预处理：letterbox 到 `640x640`，填充值 `114`；RGB、CHW、`float32 / 255`。
- NMS：置信度阈值 `0.35`、IoU 阈值 `0.45` 来自 `DEFAULT_INFER_CONFIG`。
- 坐标还原：`scale_coords()` 将 `640x640` 坐标映射回原图。
- 颜色复核：锁定前后会在原始 BGR ROI 上用 HSV 判断红/蓝，修正类别。
- 跟踪与选择：使用 Kalman 跟踪、IoU 匹配、距离/角度/面积加权评分、粘性锁定和速度前馈预测。

## 部署后端

- 目标平台：README 指向昇腾 Atlas 200I DK A2。
- 运行方式：Python 多线程，AI 线程初始化 `InferSession`，控制线程通过 RoboMaster SDK 下发云台和发射指令。
- 模型路径风险：归档目录中模型在 `models/` 下只有标签文件，但运行配置使用根目录相对路径 `16.om` / `labels.txt`。

## 未知与核验缺口

- `16.om` 未归档，无法核验网络结构、输入名、输出名、精确输出形状、算子和量化情况。
- README 提到“基于 YOLO”，源码也按 YOLO 后处理解析，但不能证明具体 YOLO 版本。
- 未提供训练权重、导出脚本、样例输入输出或硬件运行日志。
- `labels.txt` 在归档路径和运行路径之间存在位置差异，实际部署时需确认工作目录和文件摆放。
