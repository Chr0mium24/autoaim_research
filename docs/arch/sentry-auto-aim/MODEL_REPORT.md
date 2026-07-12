# sentry-auto-aim 模型报告

## 模型清单

- 已归档 [models/armor_detector/lenet.onnx](models/armor_detector/lenet.onnx) 和 [models/armor_detector/label.txt](models/armor_detector/label.txt)：装甲板数字分类，代码默认加载。
- 已归档 [models/armor_detector/mlp.onnx](models/armor_detector/mlp.onnx)：同目录备用分类模型；当前源码没有发现默认引用。
- 已归档 [models/rune_detector/yolox_rune_3.6m.onnx](models/rune_detector/yolox_rune_3.6m.onnx)，以及 OpenVINO IR [models/rune_detector/yolox_rune_3.6m.xml](models/rune_detector/yolox_rune_3.6m.xml) + `.bin`：配置默认引用 `yolox_rune_3.6m.onnx`。
- 已归档 [models/rune_detector/yolox_rune.onnx](models/rune_detector/yolox_rune.onnx)，以及 [models/rune_detector/yolox_rune.xml](models/rune_detector/yolox_rune.xml) + `.bin`：非默认能量机关模型。
- 配置使用 `package://.../model/...` 路径，归档路径在 `models/...`，直接运行需经过 ROS2 安装或改路径。

## 装甲板数字分类：lenet.onnx / mlp.onnx

- 任务：传统灯条配对得到装甲板 ROI 后，分类装甲板数字/目标类型。
- 格式/后端：ONNX，使用 OpenCV DNN `readNetFromONNX`，见 [code/rm_auto_aim/armor_detector/src/number_classifier.cpp](code/rm_auto_aim/armor_detector/src/number_classifier.cpp)。
- 输入：小装甲先透视到 `32x28`，大装甲到 `54x28`；取中间 `20x25` ROI，灰度化、Otsu 二值化、resize 到 `28x28`，再除以 255 并 `blobFromImage`。
- 输出：代码把 `forward()` 结果 reshape 成一维，取最大置信度下标，再查 [models/armor_detector/label.txt](models/armor_detector/label.txt)。
- 类别：`1, 2, 3, 4, 5, outpost, sentry, base, negative`。
- 过滤：`classifier_threshold` 默认配置为 0.7；`ignore_classes` 默认忽略 `negative`；代码还会过滤装甲板尺寸与类别不匹配的结果。
- 设计特征：神经网络只做数字分类，装甲板候选、颜色、角点主要来自传统几何。

## 能量机关：yolox_rune / yolox_rune_3.6m

- 任务：检测能量机关 5 个关键点，并分类颜色和扇叶状态。
- 格式/后端：ONNX 或 OpenVINO IR，使用 OpenVINO Runtime，见 [code/rm_rune/rune_detector/src/rune_detector.cpp](code/rm_rune/rune_detector/src/rune_detector.cpp)。
- 输入：节点先把 ROS 图像转为 `rgb8`，代码 letterbox 到 `480x480`、填充值 114；`blobFromImage(..., swapRB=true)` 生成 HWC->NCHW 的 `{1,3,480,480}` blob，注释说明该模型不需要归一化。
- 输出：IR XML 显示输出为 `{1,4725,15}`；代码按 5 点坐标、confidence、2 个颜色分数、2 个类别分数解析。旧代码注释里的 `3549 x 21` 与当前 XML/解析逻辑不一致。
- 关键点：`r_center, bottom_left, top_left, top_right, bottom_right`。
- 类别/颜色：`RuneType` 为 `INACTIVATED/ACTIVATED`；颜色映射注明训练时颜色反了，代码映射 0=Blue、1=Red。
- 后处理：stride 8/16/32 网格解码，置信度阈值默认 0.5，TopK 默认 128，NMS 默认 0.3；高重叠同类同色候选会合并关键点。之后按当前敌方颜色过滤，优先选未激活扇叶。
- 传统修正：`detect_r_tag=true` 时，用网络 R 标点做先验，在 200x200 ROI 内 Otsu、膨胀、轮廓包含关系重新估计 R 标中心。

## 预处理/后处理总览

- 装甲板链路：灰度阈值、轮廓、灯条颜色统计、灯条配对、数字分类、PCA 角点修正、PnP、BA yaw 优化。
- 能量机关链路：letterbox、OpenVINO 推理、YOLOX 解码、NMS/合并、R 标传统识别、发布 `RuneTarget`。

## 部署后端

- 装甲板数字分类使用 OpenCV DNN；没有显式设置 CUDA/OpenVINO backend。
- 能量机关使用 OpenVINO，配置 [configs/node_params/rune_detector_params.yaml](configs/node_params/rune_detector_params.yaml) 默认 `device_type: AUTO`；代码仅在设备名等于 `GPU` 时把输入输出 precision 设为 f16，否则用 f32。
- ROS2 Humble 组件化部署，图像来自 `image_raw`，结果发布到 `armor_detector/armors` 和 `rune_detector/rune_target`。

## 未知与验证缺口

- `mlp.onnx` 的输入/输出细节未从源码确认，当前也未发现默认加载路径。
- `yolox_rune` 与 `yolox_rune_3.6m` 的训练数据、backbone 差异和精度指标未归档。
- 未用 ONNX 工具解析 `lenet.onnx/mlp.onnx`，分类器真实图输入名和输出 shape 未验证。
- package 安装后的 `model` 目录映射未在本报告中实际构建验证。
