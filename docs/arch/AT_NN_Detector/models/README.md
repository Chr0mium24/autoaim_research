# 河北科技大学 Actor&Thinker 战队 YOLO26 ONNX 端到端及非端到端模型包

基于 [Ultralytics](https://github.com/ultralytics/ultralytics) YOLO26n-Pose 架构的装甲板姿态检测模型，提供两组P5注意力方案（C2PSA / CoordAttn）、两种输入分辨率（640×640 / 576×768），共 4 个 ONNX 推理模型，以及一个经过QAT+PTQ w8a16量化的用于部署在axera650开发板上的CoordAttn模型。

ps:这里提供两个模型是考虑到主流自瞄相机采用的是CS016-10UC，所以使用576x768的网络输入时 1440x1080的输入只需要进行一次resize的下采样即可，不会出现使用letterbox的padding，从而浪费带宽和算力。
## 识别链路速率

Orin NX Super 可达 200FPS (没优化)
Axera650(18TOPS) 可达 170FPS 推理可达700fps
CPU推理未测试，未来也不会考虑对CPU推理的支持(但是很有可能是能用的)

识别效果不错，RECALL非常高，对于光照的适应尤其好。可以看看video中的视频
2026北部区域赛中我们没有调过曝光，一上场就可以用。具体还是交给大家进行测试，大家说好才算好。

## 训练设备

NVIDIA GeForce RTX 3090 24GB * 2

## 模型文件一览

| 文件名 | 注意力 | 导出模式 | 输入尺寸 | 输出形状 | 大小 |
|--------|--------|----------|----------|----------|------|
| `praysky_c2psa_e2e_0228_640x640.onnx` | C2PSA | End-to-End | 1×3×640×640 | 1×30×18 | 4.3 MB |
| `praysky_c2psa_e2e_0228_576x768.onnx` | C2PSA | End-to-End | 1×3×576×768 | 1×30×18 | 4.4 MB |
| `praysky_coord_noe2e_0331_640x640.onnx` | CoordAtt | NMS | 1×3×640×640 | 1×30×18 | 5.7 MB |
| `praysky_coord_noe2e_0331_576x768.onnx` | CoordAtt | NMS | 1×3×576×768 | 1×30×18 | 5.7 MB |

> 文件名格式：`praysky_{注意力}_{是否e2e}_{训练结束日期}_{H}x{W}.onnx`

## 任务描述

检测图像中的机器人装甲板，输出：

- **检测框**（x₁, y₁, x₂, y₂）
- **目标分类**（12 类，见下表）
- **颜色属性**（4 类：蓝 B / 红 R / 灰 G / 紫 P）
- **关键点**（4 个角点，每个 2D 坐标，顺时针排列）

### 目标类别

| ID | 名称 | ID | 名称 | ID | 名称 | ID | 名称 |
|----|------|----|------|----|------|----|------|
| 0 | s0_o0 | 3 | s0_o4 | 6 | s1_o0 | 9 | s1_o4 |
| 1 | s0_o2 | 4 | s0_o5 | 7 | s1_o1 | 10 | s1_o5 |
| 2 | s0_o3 | 5 | s0_o6 | 8 | s1_o3 | 11 | s1_o7 |

- s0 : size 0(小装甲板目标)
- s1 : size 1(大装甲板目标)
- oX : objectX 0->Sentry 1->One 2->Two 3->Three 4->Four 5->Five 6->Outpost 7->base

覆盖RM2021区域赛->RM2025区域赛所有目标，RM2026中的*基地小装甲*未覆盖。该网络目前不会识别*基地小装甲*，且在w8a16量化下识别Outpost存在一定异常（极少发生且不影响拟合）初步推断为数据集中存在脏数据。也许下赛季会修复。

另注：覆盖所有参赛环境的意思是 会识别轨道哨兵，5号小装甲步兵，以及3 4 5号大装甲步兵。

## 输入格式

所有模型输入名称为 `images`，格式 **RGB**，归一化至 [0, 1]：

```
名称:   images
形状:   (batch, 3, H, W)
类型:   float32
值域:   [0.0, 1.0]
色彩:   RGB（注意：OpenCV 读取需 BGR → RGB 转换）
预处理: Letterbox Resize + Pad (fill=114)
```

### Letterbox 预处理示例

```python
import cv2
import numpy as np

def preprocess(img_bgr, target_h, target_w):
    """Letterbox 预处理：等比缩放 + 填充。"""
    h, w = img_bgr.shape[:2]
    scale = min(target_w / w, target_h / h)
    nw, nh = int(w * scale), int(h * scale)
    resized = cv2.resize(img_bgr, (nw, nh))
    canvas = np.full((target_h, target_w, 3), 114, dtype=np.uint8)
    pad_t, pad_l = (target_h - nh) // 2, (target_w - nw) // 2
    canvas[pad_t:pad_t+nh, pad_l:pad_l+nw] = resized

    # BGR → RGB → float32 → NCHW
    blob = cv2.cvtColor(canvas, cv2.COLOR_BGR2RGB).astype(np.float32) / 255.0
    blob = blob.transpose(2, 0, 1)[None]           # (1, 3, H, W)
    return blob, scale, (pad_l, pad_t)

def map_back(coords, scale, pad):
    """将模型输出的坐标映射回原始图像。"""
    pad_l, pad_t = pad
    return (coords - (pad_l, pad_t)) / scale
```

## 输出格式

所有模型统一输出形状：`(1, 30, 18)`

| 维度 | 含义 | 说明 |
|------|------|------|
| [0:4] | x₁, y₁, x₂, y₂ | 检测框（Letterbox 坐标） |
| [4] | confidence | 目标置信度 |
| [5] | class_id | 目标类别索引 (0-11) |
| [6:10] | color_scores | 4 类颜色概率 (B, R, G, P)，取 argmax 获得颜色 |
| [10:18] | kpts | 4 个关键点 (k1x, k1y, k2x, k2y, k3x, k3y, k4x, k4y) |


### 解码示例

```python
def decode(output, conf_thres=0.25):
    """解析模型输出，返回过滤后的检测结果。"""
    dets = []
    for i in range(output.shape[1]):
        d = output[0, i]
        if d[4] < conf_thres:
            continue

        box = d[0:4]                       # x1, y1, x2, y2
        conf = float(d[4])                 # 置信度
        cls_id = int(d[5])                 # 类别 ID
        color_id = int(np.argmax(d[6:10])) # 颜色类别 (0=B, 1=R, 2=G, 3=P)
        kpts = d[10:18].reshape(4, 2)      # 4 个关键点坐标

        dets.append(dict(box=box, conf=conf, cls_id=cls_id,
                         color_id=color_id, kpts=kpts))
    return dets
```

## 两组模型的区别

### C2PSA（End-to-End）

- P5 层使用 **C2PSA** 注意力（多头自注意力 PSA）
- **端到端导出**：模型内置 TopK 后处理，ONNX 推理直接输出最终结果
- 参数量更少（~164 层），模型更小
- 同样输出颜色属性分支（B/R/G/P 4 类）
- ⚠️ C2PSA 中的 SoftMax 在 uint16/uint8 量化时会有精度损失

### CoordAtt（NMS）

- P5 层使用 **CoordAtt** 注意力（通道 + 空间注意力）
- **NMS 导出**：模型内置 NMS 后处理，ONNX 推理直接输出最终结果
- 参数量较多（~223 层），模型稍大
- ✅ 全部为卷积 + Sigmoid 操作，**对量化友好**

## 快速验证

```bash
# 随机选一张验证集图片
python preview.py

# 指定图片
python preview.py path/to/image.jpg

# 调整置信度阈值
python preview.py --conf 0.15

# 只保存不弹窗
python preview.py --no-show

# 指定输出路径
python preview.py --output result.jpg
```

`preview.py` 会对 4 个模型分别推理同一张图，生成 2×2 对比网格图。

## 训练指标参考

| 模型 | 分辨率 | mAP50-95(B) | mAP50(B) | Precision(B) | Recall(B) |
|------|--------|-------------|----------|--------------|-----------|
| C2PSA (x_train_c2psa) | 640×640 | 0.8801 | 0.9880 | 0.9740 | 0.9504 |
| CoordAtt (x_coord) | 768×576 | 0.8717 | 0.9897 | 0.9761 | 0.9695 |

> 以上为各模型在其**训练分辨率**下的验证指标，仅供参考。

## 依赖

- ONNX Runtime ≥ 1.16
- OpenCV (`opencv-python`)
- NumPy

## 命名规则

```
praysky_{attention}_{export}_{MMDD}_{H}x{W}.onnx

attention:  c2psa          C2PSA 多头自注意力
            coord          CoordAtt 通道空间注意力

export:     e2e            End-to-End (内置 TopK)
            noe2e          NMS (内置 NMS)

MMDD:       训练结束日期 (best.pt 的修改日期)

H x W:      模型输入的高×宽
```

## 为什么我是这么做的？

纵观其它开源的装甲板神经网络，可以看到大家对部署网络都有自己的想法，其重心在修改网络本身的结构，以期提高网络的性能，是一种以网络的Params和GFLOPs为导向的设计。但我认为，这是一种很科研，很paper like的做法，因为网络本身结构能够给出的提升非常有限，比方说我们将P1保留为Conv，后面P2，P3，P4包括head/neck的卷积都换为DWConv，确实Params和GFLOPs都会下降，但是具体部署到Orin NX上的infer速度会变快吗？答案是会变慢，这是因为nvidia对于标准的Conv模块的优化更好，其它模块也是同理，GFLOPs虽然一定程度上反映了网络的推理速度，但是实际上我们还是需要具体情况具体考虑，这也是我为什么不使用shufflenet/mobilenet等等backbone的原因。优化网络结构本身不会给神经网络的识别速度和精度带来很大的提升，因此我将重心放到了**数据集/数据增强/训练方法/量化友好**这四个方面。其中，识别精度由C2PSA/CoordAttn等角点注意力机制来保证，识别速度由量化/部署平台保证。

### 数据集/数据增强
模型来自23000张优质数据集，数据集对于监督学习的最终效果是决定性的，一个网络最终的泛化性能很大程度上取决于数据集，且一个模型的bad case也很大概率是数据集中的脏数据引起的，数据集越干净的模型，误识别越少。因此，对于数据集的清洗是很有必要的，尤其是大家在交换数据集的时候也一定要注重数据集的质量，千万要清洗后再加入主线，经验来讲，一个类别大概2k张左右，就可以有很好的效果。在此基础上，我使用了Albumentations作为第三方数据增强的python库，为数据集添加Ultralytics仓库本身训练不具有的一些，更为丰富的增强，带来了明显的“提点”。这是免费的提点，虽然带来了比较重的显存负担导致我的训练速度一泻千里（bushi

### 训练方法
有人认为YOLOV8 YOLOV5的效果已经足够好，我想说是这样的，但是YOLOV8不够新，ultralytics的YOLO在不断更新，整个infra其实不算完善，时到如今其仓库也有海量的issue没有closed，训练这个模型我使用到了MuSGD，比单纯的SGD稳定的多，还用到了STAL等等，但是老旧的YOLOV8中这些新的，得到过验证的好的算法没有填补到框架中，还需自己手动实现，是不太理想的。

### 量化
将其量化到fp16自不必多说，我们将其部署到Axera650（18TOPS边缘计算）的过程比较夭寿，是笔者根据量化报表一点一点微调出来的。最终笔者探索出两种方法，首先不管哪种方法我们都得QAT，之后使用厂商提供的量化工具链Pulsar2进行PTQ和仿真。第一种方法是将P5的PSA替换为CoordAttn，因为PSA会有一个大的Conv后面紧接一个Softmax，其带来过于大的outlier是必然的，CoordAttn则是XY池化+1x1Conv+BN+Hard-Swish，都是量化友好的操作，然后我们手动处理anchor offset，将模型裁减后，把anchor offset有关的内容全部转移到CPU上执行（也就是FP32）但会带来PostProcess压力，同时CoordAttn的map50-95其实是不如PSA的（0.87/0.89）。第二种是修改forward，再LoRa，这种方法可以实现e2e并且只带来较小的精度损失，同时可以保证使用base中的C2PSA模块以保证精度。但由于技术和时间限制，我们未能尝试第二种方案，仅以第一种方案上场。

### 鸣谢

特别感谢以下几个战队/同学给予我的帮助，没有你们，我是没办法独立完成这些工作的：

- 西北工业大学 *WMJ* 战队（刘队刘队我们喜欢你，感谢高质量数据集）
- 中国石油大学（华东）*RPS* 战队（感谢李神的高质量数据集）
- 上海科技大学 *Magician* 战队（哈基星及老登提供的量化技术支持）
- 沈阳航空航天大学 *TUP* 战队（感谢爱玩棉花娃娃的顾佬点拨迷津）
- 武汉科技大学 *崇实* 战队（hy佬在我最穷的时候提供 NX 测试）
- 贵州师范大学 *CEC* 战队（感谢提供数据集，尽管未使用，祝你们早日进入 UC 舞台）
- 感谢佳怡姐，负责了训练时不少的 dirty work，感谢各位帮忙标注数据集的小登，以及负责后续维护 [数据集标注工具](https://github.com/PraySky1337/ATLabelMaster.git) 的钻石镐（byd 自定义客户端给我做好了啊）

以上排名不分先后。


## License

```
GNU AFFERO GENERAL PUBLIC LICENSE
Version 3, 19 November 2007

Copyright (C) 2007 Free Software Foundation, Inc. <https://fsf.org/>

本程序是自由软件：你可以重新分发和/或修改它，前提是遵守
由自由软件基金会发布的 GNU Affero 通用公共许可证第 3 版
（或更新版本）的条款。

本程序的分发希望它能有所帮助，但不提供任何保证；
甚至没有对适销性或特定用途适用性的默示保证。
有关更多详细信息，请参见 GNU Affero 通用公共许可证。

你应该已经随本程序收到一份 GNU Affero 通用公共许可证的副本。
如果没有，请参阅 <https://www.gnu.org/licenses/>。

---

本模型权重基于 Ultralytics (https://github.com/ultralytics/ultralytics)
AGPL-3.0 许可证训练产生。

Ultralytics 原始仓库许可证:
  https://github.com/ultralytics/ultralytics/blob/main/LICENSE

这意味着:
  - 你可以自由使用、修改和分发本模型及其衍生作品
  - 任何基于本模型的网络服务（包括 SaaS）必须开源其完整源代码
  - 衍生作品必须同样采用 AGPL-3.0 许可证
  - 必须保留原始版权声明和许可证文本
```
