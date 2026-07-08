# SHtech_auto_aim 架构报告

## 项目定位

`SHtech_auto_aim` 是上海科技大学风格的单体式自瞄工程，入口清晰，围绕 `sensor -> detect -> corner_refine -> predict -> planner -> serial` 的流水线运行。

项目支持三种推理后端：

- ONNX / ROCm
- TensorRT
- AXCL / AX650 系列

从部署角度看，它更像是一个“单进程编译开关驱动的多平台自瞄框架”。

## 模型架构

### 装甲板检测

装甲板检测模型由 `asset/models/SKD250526.onnx` 和 `asset/models/SKD250526.axmodel` 承载，`launch.cfg` 默认指向 AXCL 版本。

从 `detect/ONNX/ONNX.cpp`、`detect/TensorRT/TRTModule.cpp` 和 `detect/AXCL/AXCL.cpp` 可以看出，这个模型是一个角点型装甲板网络，输出大致包含：

- 4 个角点坐标
- 置信度
- 类别
- 颜色
- 尺寸 / 其他辅助分类

TensorRT 版本把原始输出再做一次 top-k 和 gather，加速后处理；AXCL 版本则直接读取固定形状输出并做 NMS / overlap 过滤。

### 角点修正

检测结果不是直接送给 PnP，而是先经过 `ArmorCornerOptimizer`：

- 以 YOLO 角点为中心切 ROI
- 二值化和灯条提取
- 基于灯条几何特征筛选最优边缘
- 将灯条角点重新组织成更稳定的装甲板四角

这是这套工程里很关键的“传统视觉补偿层”。

### 位姿恢复

`mathutils/CoordTransformer.cpp` 和 `detect/pnp_solver` 对应的逻辑负责 PnP 反解：

- 根据装甲板类型选取 3D 模型点
- 使用 `solvePnP` / `solvePnPGeneric` 求解位姿
- 结合重投影误差、roll 阈值和装甲板朝向挑选解

因此模型输出到跟踪器之前，已经是稳定的 3D 位姿与朝向。

## 推理后端

推理后端通过 `detect/CMakeLists.txt` 统一切换：

- `INFERENCE_BACKEND=ONNX` 时使用 MIGraphX / ROCm
- `INFERENCE_BACKEND=TRT` 时使用 TensorRT
- `INFERENCE_BACKEND=AXCL` 时使用 AXCL / AX650

`detect/backend.hpp` 把后端统一成 `operator()(cv::Mat, std::vector<bbox_t>&)` 形式，便于上层流水线复用。

## 后处理、跟踪与预测

### 检测后处理

`detect/detect_submodule.cpp` 负责把推理结果映射回原图坐标，并把角点框送往后续模块。

`corner_refine_submodule.cpp` 负责传统修正，成功时会把检测源标记为 `TRADITIONAL`，失败则保留神经网络结果。

### 目标跟踪

`predict/Tracker.cpp` 是整条跟踪链的核心，包含：

- 单装甲板卡尔曼滤波
- 整车扩展卡尔曼滤波
- 状态机切换
- 装甲板跳变检测
- 哨兵等特殊目标处理

它的目标不是只平滑一个坐标点，而是同时维护装甲板状态和整车状态。

### 云台规划

`planner/Planner.cpp` 使用 TinyMPC 做偏航和俯仰轨迹优化：

- 生成参考轨迹
- 预测子弹飞行时间
- 补偿通信与拨盘延迟
- 解两轴 MPC
- 输出开火阈值和目标角速度 / 加速度

这部分是工程的闭环核心。

## 关键配置

已复制的关键配置主要有：

- `configs/launch.cfg`：模型路径、相机输入、串口、调试开关、模块启停
- `configs/camParam/*.yml`：不同相机/镜头组合的标定参数
- `configs/plannerParam/*.yml`：不同英雄 / 步兵 / 哨兵平台的 MPC 和弹道参数

## 关键代码

已复制的关键源码路径：

- `code/detect/*`：推理后端、角点优化、预处理和检测子模块
- `code/mathutils/*`：PnP、坐标变换、EKF / KF、角度与工具函数
- `code/predict/*`：跟踪器和多策略预测器
- `code/planner/*`：TinyMPC 规划与弹道策略
- `code/sensor/*`：相机和输入源封装
- `code/timedserial/*`：串口和时序通信
- `code/entrystage/*`：系统入口编排
- `code/common/*`：流水线和公共数据结构
- `code/main.cpp`、`code/main.hpp`：总启动流程和线程生命周期管理
- `code/build.sh`、`code/install_service.py`：构建与服务部署入口

## 可复用建议

这套工程最值得复用的地方有三块：

1. 后端抽象清晰，适合做“同一套检测逻辑跨 ONNX / TensorRT / AXCL 迁移”。
2. `ArmorCornerOptimizer + PnP` 的组合很实用，适合把纯神经网络输出再稳一层。
3. `Tracker + TinyMPC` 的闭环结构完整，适合直接借鉴到需要平滑控制的云台项目。

需要适配的地方也很明确：

- 角点顺序和类别映射
- 相机标定文件
- TinyMPC 的状态维度与代价矩阵
- 串口协议和任务模式枚举

## 本次归档说明

本目录已整理出：

- `configs/`
- `code/`
- `models/`

其中 `models/` 保留了小体积模型文件，用于说明部署形态，不包含训练数据和构建产物。

