# SHtech_auto_aim 模型报告

## 模型清单

- [models/SKD250526.onnx](models/SKD250526.onnx)：ONNX，输入 `image` fp32 `[1,3,512,640]`，输出 `out` fp32 `[1,6720,21]`。
- [models/SKD250526.axmodel](models/SKD250526.axmodel)：AXCL/Axera 部署模型；二进制字符串中可见输出 `out` 为 FP32 `[1,6720,21]`。
- [models/SZU0526_fp32input_512x640_nopre_fixoutput.onnx](models/SZU0526_fp32input_512x640_nopre_fixoutput.onnx)：ONNX，输入 `images_fp32` fp32 `[1,3,512,640]`，输出 `output` fp32 `[1,20160,22]`。
- 默认运行配置：[configs/launch.cfg](configs/launch.cfg) 写的是原工程路径 `asset/models/SKD250526.axmodel`；归档目录实际放在 `models/` 下。
- 未归档：训练权重、导出脚本、TensorRT cache/engine、模型专用 README。

## 模型细节

### `SKD250526.onnx` / `SKD250526.axmodel`

- 任务：装甲板四角点检测。
- 输入尺寸与布局：公共预处理输出 `640x512` RGB 图像；ONNX/TensorRT 使用 NCHW fp32 归一化输入，AXCL 代码直接复制 HWC uint8 RGB 输入。
- 输出布局：`6720 x 21`。TRT/AXCL 后处理按每个候选 21 个值解析：`0:8` 四角点，`8` 置信度，`9:17` 8 类 tag logits，`17:19` 2 类颜色 logits，`19:21` 2 类装甲尺寸 logits。
- 类别：源码只证明 8 个 tag logits；写入 `bbox_t.tag_id` 后在下游按 `1-7 robot, 8 outpost, 9 base` 语义使用，精确训练标签名未在模型文件旁给出。
- 颜色：2 类颜色 logits，写入 `bbox_t.color_id`；公共类型注释中 `0=red, 1=blue, 2=gray`。
- 角点顺序：TRT/AXCL 都会交换第 3、4 个点，把“左上、左下、右上、右下”调整为旧代码期望的顺序。

### `SZU0526_fp32input_512x640_nopre_fixoutput.onnx`

- 任务：同类装甲板四角点检测。
- 输入：`images_fp32` fp32 `[1,3,512,640]`。
- 输出：`20160 x 22`，与 [code/detect/ONNX/ONNX.cpp](code/detect/ONNX/ONNX.cpp) 的硬编码解析一致。
- 输出布局：`0:8` 四角点，`8` 置信度 logit，`9:13` 4 类颜色 logits，`13:22` 9 类目标 logits。
- 状态：未被 [configs/launch.cfg](configs/launch.cfg) 默认引用，更像 ONNX/ROCm 后端的备选模型。

## 预处理与后处理

- 公共预处理：[code/detect/preprocess_submodule.cpp](code/detect/preprocess_submodule.cpp) 将原图直接 resize 到 `640x512`，再 `BGR -> RGB`；不做 letterbox。
- 坐标映射：[code/detect/detect_submodule.cpp](code/detect/detect_submodule.cpp) 按 `scale_x/scale_y` 将检测点映射回原图。
- ONNX/ROCm 后端：MIGraphX 加载 ONNX/cache，`blobFromImage(..., 1/255)`，置信度 logit 阈值 `0.619`，NMS 阈值 `0.45`。
- TensorRT 后端：解析 ONNX 后在网络内对置信度做 TopK，输出 `output-topk`，最多保留 128 个候选，再做 overlap 去重。
- AXCL 后端：加载 `.axmodel`，复制 RGB uint8 输入，遍历 6720 个候选，按网格/stride 解码点坐标，阈值 `0.1` 后做 overlap 去重。
- 角点修正：[code/detect/corner_refine_submodule.cpp](code/detect/corner_refine_submodule.cpp) 对神经网络角点做传统视觉灯条修正，成功时将来源标为 `TRADITIONAL`。

## 部署后端

- 默认构建：[code/CMakeLists.txt](code/CMakeLists.txt) 默认 `INFERENCE_BACKEND=AXCL`。
- 可选后端：[code/detect/CMakeLists.txt](code/detect/CMakeLists.txt) 支持 `ONNX`(MIGraphX/ROCm)、`TRT`(TensorRT/CUDA)、`AXCL`(Axera/Ax650)。
- 默认运行：[configs/launch.cfg](configs/launch.cfg) 指向 `SKD250526.axmodel`，即 AXCL 路径。

## 未知与核验缺口

- 缺少模型 README、训练配置和标签表；`SKD250526` 的 8 类 tag 精确语义只能从下游 `tag_id` 注释间接推断。
- `SKD250526.onnx` 和 `SKD250526.axmodel` 的点坐标解码在 TRT 与 AXCL 代码中不完全相同，需要用同一张图做后端一致性验证。
- `SZU0526...onnx` 未被默认配置引用，实际是否仍在用需结合构建参数和启动配置确认。
- 未在目标硬件上运行 AXCL/TensorRT/MIGraphX，性能、量化误差和 IO 名称兼容性未验证。
