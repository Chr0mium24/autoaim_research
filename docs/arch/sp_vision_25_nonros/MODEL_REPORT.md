# sp_vision_25_nonros 模型报告

## 模型清单

| 文件 | 本地状态 | 代码/配置引用 | 备注 |
| --- | --- | --- | --- |
| [assets/yolov5.xml](assets/yolov5.xml) | 已复制 XML | `configs/*.yaml` 引用 | 主自瞄默认模型；[models/yolov5.xml](models/yolov5.xml) 是重复副本。 |
| [assets/yolov8.xml](assets/yolov8.xml) | 已复制 XML | `configs/*.yaml` 引用 | 可选主自瞄模型；[models/yolov8.xml](models/yolov8.xml) 是重复副本。 |
| [assets/yolo11.xml](assets/yolo11.xml) | 已复制 XML | `configs/*.yaml` 引用 | 可选主自瞄模型；[models/yolo11.xml](models/yolo11.xml) 是重复副本。 |
| [assets/tiny_resnet.onnx](assets/tiny_resnet.onnx) | 已复制 ONNX | `classify_model: assets/tiny_resnet.onnx` | 装甲数字分类器；[models/tiny_resnet.onnx](models/tiny_resnet.onnx) 是重复副本。 |
| [assets/yolo11_buff_int8.xml](assets/yolo11_buff_int8.xml) | 已复制 XML | 多个打符配置引用 | 打符 6 点模型；[models/yolo11_buff_int8.xml](models/yolo11_buff_int8.xml) 是重复副本。 |
| [assets/standard_fanblade.jpg](assets/standard_fanblade.jpg) | 已复制 JPG | `demo.yaml`、`mvs.yaml` 引用 | 配置层传统打符模板；已查模型链路未见读取。 |
| [assets/best2-sim.onnx](assets/best2-sim.onnx) | 已复制 ONNX | 未找到引用 | 任务和输入输出未知。 |
| [models/mpc_layout.xml](models/mpc_layout.xml)、[models/buff_layout.xml](models/buff_layout.xml) | 已复制 XML | 布局/可视化 | 不是神经网络模型。 |

未发现 `.bin` 文件。所有 OpenVINO IR XML 中都有 `data offset/size` 常量引用，需补齐同名 `.bin` 或实际验证 OpenVINO 是否能单独加载这些 XML。

## 主自瞄模型

入口是 [tasks/auto_aim/yolo.cpp](tasks/auto_aim/yolo.cpp)，配置通过 `yolo_name` 选择模型。`code/auto_aim/` 下还有一份相近代码副本，模型语义以 `tasks/` 为准。

### yolov5.xml

- 任务：装甲板检测，含颜色、数字/名称和 4 点。
- 格式：OpenVINO IR XML；XML 输入 `[1,3,640,640]`，输出 `[1,25200,22]`。
- 预处理：[yolov5.cpp](tasks/auto_aim/yolos/yolov5.cpp) 做 `640x640` 左上 letterbox；OpenVINO 预处理声明 NHWC BGR `u8`，转 NCHW RGB，除以 255。
- 输出解析：每行包含 8 个角点坐标、objectness、4 个颜色分数、9 个数字/名称分数；objectness 过 sigmoid。
- 后处理：NMS、名称/大小合法性过滤；`use_traditional` 为真时用传统灯条检测修角点。

### yolov8.xml

- 任务：B/R 两类候选 + 4 点，装甲名称由分类器补充。
- 格式：OpenVINO IR XML；XML 输入 `[1,3,416,416]`，输出 `[1,14,3549]`。
- 元数据：Ultralytics pose，`imgsz [416,416]`，`names {0:'B',1:'R'}`，`kpt_shape [4,2]`。
- 输出解析：`4 bbox + 2 class + 4*2 keypoints`；4 点排序后构造 `Armor`。
- 后处理：裁剪图案，调用 `tiny_resnet` 分类，再做类型过滤；该版本 YOLOV8 不读取 `use_traditional`。

### yolo11.xml

- 任务：组合类别装甲检测，直接输出颜色/名称/大小装甲语义。
- 格式：OpenVINO IR XML；XML 输入 `[1,3,640,640]`，输出 `[1,50,8400]`。
- 输出解析：`4 bbox + 38 class + 4*2 keypoints`；38 类来自 [armor.hpp](tasks/auto_aim/armor.hpp) 的 `armor_properties`。
- 后处理：最大类分数、NMS、4 点排序、名称/类型过滤；未见传统修正调用。

## 分类器

[assets/tiny_resnet.onnx](assets/tiny_resnet.onnx) 由 [classifier.cpp](tasks/auto_aim/classifier.cpp) 读取。实际 `classify()` 使用 OpenCV DNN：灰度图案 letterbox 到 `32x32`，归一化，输出 9 类 softmax。代码也编译了 OpenVINO `AUTO` 模型，但 `ovclassify()` 未在检测链路中看到调用。

## 打符模型

[assets/yolo11_buff_int8.xml](assets/yolo11_buff_int8.xml) 是打符链路引用模型。

- 任务：能量机关目标检测 + 6 点关键点。
- 格式：OpenVINO IR XML；XML 输入 `[1,3,640,640]`，输出 `[1,17,8400]`。
- 代码语义：`NUM_POINTS = 6`；单候选路径按 `4 bbox + 1 score + 6*2 keypoints` 解析。
- 后端：`YOLO11_BUFF` 固定 `compile_model(model, "CPU")`。
- 预处理：单候选路径用 `fill_tensor_data_image()` 做等比 `warpAffine`，BGR->RGB，归一化，NCHW 写入。
- R 中心：传统补偿从第 5/6 点外推 R 中心，再在灰度阈值、膨胀后的 mask 内找轮廓细化。
- 多候选缺口：`get_multicandidateboxes()` 注释和索引仍像旧 `[15,8400]` 版本，和 XML `[1,17,8400]` 不完全一致；需用真实模型输出验证。

## 其他资产

- [assets/best2-sim.onnx](assets/best2-sim.onnx)：未找到配置或 C++ 引用，无法确认任务、输入大小和输出含义。
- [assets/standard_fanblade.jpg](assets/standard_fanblade.jpg)：配置引用为传统打符模板，但当前 `tasks/auto_buff` 代码没有直接读取。
- `models/` 下的模型多为 `assets/` 副本；配置实际指向 `assets/`。

## 部署后端

- CMake 固定 OpenVINO `/opt/intel/openvino_2024.6.0/runtime/cmake/`。
- 主自瞄 YOLO 使用 YAML 的 `device`，常见 `CPU`/`GPU`；打符固定 CPU；分类器实际用 OpenCV DNN。
- 多线程 detector 固定 `{1,640,640,3}` 输入，适配 `yolov8.xml` 的 `416x416` 形状需要验证。

## 未知与验证缺口

- 所有 IR 缺同名 `.bin`，模型加载未验证。
- `tiny_resnet.onnx` 和 `best2-sim.onnx` 未用 ONNX 解析器读取；除分类器代码外，不写架构结论。
- 打符多候选路径与 XML 输出形状不一致，需跑样例确认。
- `standard_fanblade.jpg` 是否仍参与部署未由代码证明。
