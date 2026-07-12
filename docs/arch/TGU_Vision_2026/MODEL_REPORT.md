# TGU_Vision_2026 模型报告

## 模型清单

| 文件 | 本地状态 | 代码/配置引用 | 备注 |
| --- | --- | --- | --- |
| [models/yolov5.xml](models/yolov5.xml) | 已复制 XML | `configs/*.yaml` 引用 `assets/yolov5.xml` | 主自瞄默认模型；本目录缺 `assets/` 副本。 |
| [models/yolov8.xml](models/yolov8.xml) | 已复制 XML | `configs/*.yaml` 引用 `assets/yolov8.xml` | 可选主自瞄模型。 |
| [models/yolo11.xml](models/yolo11.xml) | 已复制 XML | `configs/*.yaml` 引用 `assets/yolo11.xml` | 可选主自瞄模型。 |
| [models/tiny_resnet.onnx](models/tiny_resnet.onnx) | 已复制 ONNX | `classify_model: assets/tiny_resnet.onnx` | YOLOV8/传统检测用数字分类器。 |
| [models/yolo11_buff_int8.xml](models/yolo11_buff_int8.xml) | 已复制 XML | `standard/ascento.yaml` 引用 `assets/yolo11_buff_int8.xml` | 打符 6 点模型。 |
| [models/standard_fanblade.jpg](models/standard_fanblade.jpg) | 已复制 JPG | `demo.yaml` 引用 `./assets/standard_fanblade.jpg` | 只在配置中出现，未在已查代码中看到读取。 |
| [configs/mpc_layout.xml](configs/mpc_layout.xml)、[configs/buff_layout.xml](configs/buff_layout.xml) | 已复制 XML | UI/布局配置 | 不是神经网络模型。 |

未发现 `.bin` 文件。IR XML 中有 `data offset/size` 常量引用，需补同名 `.bin` 或实测确认可单文件加载。配置统一指向 `assets/...`，但本项目目录只放了 `models/...`。

## 主自瞄模型

入口是 [code/tasks/auto_aim/yolo.cpp](code/tasks/auto_aim/yolo.cpp)，按 `yolo_name` 选择 `YOLOV5/YOLOV8/YOLO11`。同步链路使用 OpenVINO `LATENCY`；多线程链路 [mt_detector.cpp](code/tasks/auto_aim/multithread/mt_detector.cpp) 使用 `THROUGHPUT`。

### yolov5.xml

- 任务：装甲板检测、颜色/数字识别、4 点输出。
- 格式：OpenVINO IR XML；XML 输入 `[1,3,640,640]`，输出 `[1,25200,22]`。
- 预处理：`640x640` 左上 letterbox 黑底；NHWC BGR `u8` -> NCHW RGB `f32/255`。
- 输出解析：8 个角点坐标、objectness、4 个颜色分数、9 个数字/名称分数；objectness 过 sigmoid。
- 后处理：NMS `0.3`、分数阈值 `0.7`、`min_confidence`、名称和大小装甲合法性过滤。
- 传统修正：代码支持 `use_traditional` 时调用 [detector.cpp](code/tasks/auto_aim/detector.cpp) 从网络框附近重新找灯条修角点；当前多数实机配置为 `false`，`example.yaml` 为 `true`。

### yolov8.xml

- 任务：B/R 两类装甲候选 + 4 点，名称由分类器补充。
- 格式：OpenVINO IR XML；XML 输入 `[1,3,416,416]`，输出 `[1,14,3549]`。
- 元数据：Ultralytics pose，`imgsz [416,416]`，`names {0:'B',1:'R'}`，`kpt_shape [4,2]`。
- 输出解析：转置后按 `4 bbox + 2 class + 4*2 keypoints` 解析；关键点排序为左上、右上、右下、左下。
- 后处理：裁剪灯条扩展 ROI，调用 `tiny_resnet` 分类名称，再做类型过滤。
- 差异点：TGU 版 YOLOV8 构造函数未读取 `use_traditional`，解析后不调用传统修正。

### yolo11.xml

- 任务：组合类别装甲检测，直接输出颜色/名称/大小语义。
- 格式：OpenVINO IR XML；XML 输入 `[1,3,640,640]`，输出 `[1,50,8400]`。
- 输出解析：`4 bbox + 38 class + 4*2 keypoints`；38 类来自 [armor.hpp](code/tasks/auto_aim/armor.hpp) 的 `armor_properties`。
- 后处理：最大类分数、NMS、4 点排序、名称/大小装甲合法性过滤。
- 差异点：有 `Detector detector_` 成员，但 parse 中未调用传统修正。

## 分类器

[models/tiny_resnet.onnx](models/tiny_resnet.onnx) 在 [classifier.cpp](code/tasks/auto_aim/classifier.cpp) 中同时被 OpenCV DNN 和 OpenVINO 读取。实际 `classify()` 使用 OpenCV DNN：图案转灰度，letterbox 到 `32x32`，归一化后输出 9 类 softmax；`ovclassify()` 存在但未在主链路中看到调用。

## 打符模型

[models/yolo11_buff_int8.xml](models/yolo11_buff_int8.xml) 是当前配置引用的打符模型。

- 任务：能量机关目标检测 + 6 点关键点。
- 格式：OpenVINO IR XML；XML 输入 `[1,3,640,640]`，输出 `[1,17,8400]`。
- 代码语义：`NUM_POINTS = 6`，单候选路径按 `4 bbox + 1 score + 6*2 keypoints` 解析；`class_names={"buff","r"}` 只用于标签文本。
- 后端：`YOLO11_BUFF` 固定 `compile_model(model, "CPU")`。
- 单候选预处理：`fill_tensor_data_image()` 做保持比例的 `warpAffine`，BGR->RGB，归一化，写入 NCHW `f32`。
- R 中心修正：[buff_detector.cpp](code/tasks/auto_buff/buff_detector.cpp) 用第 5/6 点外推 R 中心，再用灰度阈值、膨胀、mask 内轮廓细化。
- 多候选缺口：`get_multicandidateboxes()` 的注释仍写 `[15,8400]`，且局部 NHWC tensor 未设置到 infer request；与 XML `[1,17,8400]` 不完全一致，需要运行验证。

## 部署后端

- CMake 固定 OpenVINO `/opt/intel/openvino_2024.6.0/runtime/cmake/`。
- YOLO 同步链路设备来自 YAML，常见 `GPU`/`CPU`；打符固定 CPU；分类器实际走 OpenCV DNN。
- 多线程 detector 固定输入 tensor shape `{1,640,640,3}`，对 `yolov8.xml` 的 `416x416` 形状是否可用未验证。

## 未知与验证缺口

- 缺 `.bin` 权重和 `assets/` 路径副本，是当前最大部署缺口。
- `yolo11.xml`、`yolo11_buff_int8.xml` XML 没有可读框架元数据，类别语义主要来自 C++ 代码。
- `tiny_resnet.onnx` 未被 ONNX 解析器读取，只从代码确认输入和 9 类输出。
- `standard_fanblade.jpg` 在本地有文件，但未找到代码读取点；可能是旧传统打符配置残留。
