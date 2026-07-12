# Model report expansion plan

Date: 2026-07-12

## Goal

Add a dedicated `MODEL_REPORT.md` under every project folder in `docs/arch/`.

Each report should focus on model assets and model design rather than the whole system architecture. The reports must include what can be confirmed from local files, and explicitly mark unknowns when the repository does not expose enough model metadata.

## Scope

Project folders:

- `26-orin-Gimbal-AutoAim`
- `AIRS-RM-2025`
- `AT_NN_Detector`
- `Climber_Vision_26`
- `RP_Infantry_Plus`
- `RobotDetectionModel`
- `SHtech_auto_aim`
- `TGU_Vision_2026`
- `breeze`
- `jlu_vision_26`
- `robomaster-cv`
- `sentry-auto-aim`
- `sp_vision_25_nonros`

## Report Format

Each `MODEL_REPORT.md` should cover:

1. Model inventory: copied model files, referenced model files, and missing files if applicable.
2. Per-model details: task, format, input size or layout, output layout, classes/keypoints, and known design features.
3. Preprocessing and postprocessing: resize/letterbox, color conversion, normalization, NMS, keypoint remap, classifier use, or traditional CV fallback.
4. Deployment backend: ONNX Runtime, OpenVINO, TensorRT, AXCL/Axera, Darknet/OpenCV DNN, Ascend, DeepStream, or no neural model.
5. Gaps and verification notes: exact shapes or architecture fields that cannot be proven from local code/files.

## Delegation

Use subagents with disjoint write ownership:

- Worker A: `AIRS-RM-2025`, `AT_NN_Detector`, `RobotDetectionModel`, `SHtech_auto_aim`
- Worker B: `Climber_Vision_26`, `TGU_Vision_2026`, `sp_vision_25_nonros`
- Worker C: `jlu_vision_26`, `sentry-auto-aim`, `robomaster-cv`
- Worker D: `26-orin-Gimbal-AutoAim`, `RP_Infantry_Plus`, `breeze`

The main agent will review outputs, write results, commit, and push.
