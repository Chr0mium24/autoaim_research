# jlu_vision_26 模型报告

## 模型清单

- 已归档 [models/0526.onnx](models/0526.onnx)：装甲板四角点检测模型。配置 [configs/auto_aim/armor_detector.yaml](configs/auto_aim/armor_detector.yaml) 引用运行时路径 `assets/0526.onnx`。
- 已归档 [models/yolox_rune_3.6m.onnx](models/yolox_rune_3.6m.onnx)：能量机关扇叶/关键点模型。配置 [configs/auto_buff/buff_detector.yaml](configs/auto_buff/buff_detector.yaml) 引用运行时路径 `assets/yolox_rune_3.6m.onnx`。
- 未归档 OpenVINO IR、TensorRT engine、标签文件或训练配置。若直接用本归档运行，需要把 `assets/...` 路径改到 `models/...`，或补齐 `assets` 目录。

## 装甲板模型：0526.onnx

- 任务：检测装甲板并输出四个灯条角点、颜色、装甲类型和置信度。
- 格式/后端：ONNX，由 [code/src/armor_detector/src/yolo.cpp](code/src/armor_detector/src/yolo.cpp) 通过 OpenVINO `read_model` 加载。
- 输入：代码构造 `u8` NHWC/BGR tensor，形状 `{1,640,640,3}`；OpenVINO 预处理转为 `f32`、RGB、模型侧 NCHW，并除以 255。原图按比例缩放到左上角，剩余区域黑色填充。
- 输出：代码按 `output_shape[1] x output_shape[2]` 的 `float` 矩阵逐行解析。每行列含义为 0-7 四个角点坐标，8 为置信度 logit，9-12 为颜色分数，13-21 为 9 类装甲类型分数。
- 类别：硬编码映射为 `Sentry, One, Two, Three, Four, Negative, Outpost, Base, Negative`。颜色映射只明确 `Blue, Red, Extinguished`。
- 后处理：sigmoid 置信度筛选，`NMSBoxes`，`accept_thresh` 过滤，无效装甲过滤；角点重排为左下、左上、右上、右下后构造灯条；再按敌我颜色过滤，PCA/矩分析角点修正，PnP 解算并发布 iceoryx `armors`。
- 设计特征：深度模型直接给四角点，传统几何只做角点精修和位姿恢复。

## 能量机关模型：yolox_rune_3.6m.onnx

- 任务：检测能量机关扇叶，输出 R 标中心与扇叶四点、颜色、扇叶状态和置信度。
- 格式/后端：ONNX，由 [code/src/auto_buff/buff_detector/src/yolo.cpp](code/src/auto_buff/buff_detector/src/yolo.cpp) 通过 OpenVINO 加载。
- 输入：`f32` NHWC/BGR tensor，形状 `{1,480,480,3}`；OpenVINO 预处理转 RGB、模型侧 NCHW。原图 letterbox 到 480x480，边界填充值 114，不做 `/255` 归一化。
- 输出：代码按 5 个点、1 个 confidence、2 个颜色、2 个类别解析，即每 anchor 15 列；stride 为 8/16/32，480 输入下可推出 4725 个 anchor。ONNX 本体未用工具解析验证。
- 关键点：`center/r_center, bottom_left, top_left, top_right, bottom_right`。颜色映射为 0=Red、1=Blue；类别映射为 0=Inactivated、1=Activated。
- 后处理：置信度阈值、TopK、NMS；同类同色且高 IoU 的候选会合并关键点并按置信度加权平均。随后按颜色监听结果过滤，`CenterCorrector` 用面积、ROI、Otsu、形态学和近正方形轮廓修正 R 标中心。
- 设计特征：YOLOX 网格解码 + 传统中心修正；大符模式会丢弃已激活扇叶。

## 部署后端

- 两个视觉模型均走 OpenVINO Runtime，设备来自配置，当前归档配置均为 `CPU`。
- 装甲板检测支持单线程/多线程异步请求；能量机关检测当前代码只写了单线程 `STDetectorDL`。
- 进程通信不是 ROS，而是 iceoryx 话题和项目内消息定义。

## 未知与验证缺口

- 未解析 ONNX 图本体，模型真实输入/输出名、动态维度和 backbone 未验证。
- `0526.onnx` 的颜色分数代码取 4 列，但颜色映射只定义 3 个值，若预测到第 4 类会有风险。
- 能量机关预处理只在首次图像记录反变换矩阵，图像尺寸变化时行为未验证。
- 训练数据、导出脚本、量化方式和精度指标未归档。
