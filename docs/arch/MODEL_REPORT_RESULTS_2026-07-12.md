# Model report expansion results

Date: 2026-07-12

## Summary

Added one dedicated `MODEL_REPORT.md` under each project folder in `docs/arch/`.

The reports focus on model inventory, per-model input/output details, deployment backend, preprocessing, postprocessing, and verification gaps. They intentionally mark unknowns where local files do not expose exact model shapes, labels, weights, or architecture metadata.

## Added Reports

| Project | Report |
| --- | --- |
| `26-orin-Gimbal-AutoAim` | `docs/arch/26-orin-Gimbal-AutoAim/MODEL_REPORT.md` |
| `AIRS-RM-2025` | `docs/arch/AIRS-RM-2025/MODEL_REPORT.md` |
| `AT_NN_Detector` | `docs/arch/AT_NN_Detector/MODEL_REPORT.md` |
| `Climber_Vision_26` | `docs/arch/Climber_Vision_26/MODEL_REPORT.md` |
| `RP_Infantry_Plus` | `docs/arch/RP_Infantry_Plus/MODEL_REPORT.md` |
| `RobotDetectionModel` | `docs/arch/RobotDetectionModel/MODEL_REPORT.md` |
| `SHtech_auto_aim` | `docs/arch/SHtech_auto_aim/MODEL_REPORT.md` |
| `TGU_Vision_2026` | `docs/arch/TGU_Vision_2026/MODEL_REPORT.md` |
| `breeze` | `docs/arch/breeze/MODEL_REPORT.md` |
| `jlu_vision_26` | `docs/arch/jlu_vision_26/MODEL_REPORT.md` |
| `robomaster-cv` | `docs/arch/robomaster-cv/MODEL_REPORT.md` |
| `sentry-auto-aim` | `docs/arch/sentry-auto-aim/MODEL_REPORT.md` |
| `sp_vision_25_nonros` | `docs/arch/sp_vision_25_nonros/MODEL_REPORT.md` |

## Subagent Split

- Worker A wrote `AIRS-RM-2025`, `AT_NN_Detector`, `RobotDetectionModel`, and `SHtech_auto_aim`.
- Worker B wrote `Climber_Vision_26`, `TGU_Vision_2026`, and `sp_vision_25_nonros`.
- Worker C wrote `jlu_vision_26`, `sentry-auto-aim`, and `robomaster-cv`.
- Worker D wrote `26-orin-Gimbal-AutoAim`, `RP_Infantry_Plus`, and `breeze`.

## Verification

- Confirmed exactly 13 `MODEL_REPORT.md` files exist under `docs/arch/*/`.
- `git diff --check` passed after subagent work.
- Reports are scoped to documentation only; no submodule pointers or copied model assets were changed.

## Notable Gaps Captured In Reports

- Several OpenVINO XML files were copied without matching `.bin` weights, so runtime loading was not verified.
- Some ONNX and OM models were not parsed with model-inspection tooling; their exact graph input/output names remain inferred from source code.
- Some upstream projects reference model files, labels, Darknet weights, Caffe weights, or DeepStream engines that are not present in the local archive.
- Non-vision/control-only projects explicitly state that no neural model is present in the archived files.
