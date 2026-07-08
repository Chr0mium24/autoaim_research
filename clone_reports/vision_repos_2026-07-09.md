# RoboMaster vision repositories clone report

Date: 2026-07-09

## Plan

Clone the RoboMaster vision, auto-aim, and related vision framework repositories discussed in the search notes into `open_source_vision_repos/`.

Use shallow clones where possible to keep the local checkout practical. The current directory is not a git repository, so no outer workspace commit can be created.

## Repository List

| Local directory | Source |
| --- | --- |
| `jlu_vision_26` | https://github.com/Fskaaaaaaaa/jlu_vision_26.git |
| `SHtech_auto_aim` | https://github.com/Astra-Whale/SHtech_auto_aim.git |
| `Climber_Vision_26` | https://github.com/CCZU-Climber/Climber_Vision_26.git |
| `sentry-auto-aim` | https://gitee.com/ustl-cod/sentry-auto-aim.git |
| `TGU_Vision_2026` | https://github.com/TGURM304/TGU_Vision_2026.git |
| `AT_NN_Detector` | https://github.com/PraySky1337/AT_NN_Detector.git |
| `26-orin-Gimbal-AutoAim` | https://github.com/LareinaWeii/26-orin-Gimbal-AutoAim.git |
| `robomaster-cv` | https://github.com/PolySTAR-mtl/robomaster-cv.git |
| `AIRS-RM-2025` | https://github.com/BlueDarkUP/AIRS-RM-2025.git |
| `RobotDetectionModel` | https://github.com/broalantaps/RobotDetectionModel.git |
| `RP_Infantry_Plus` | https://github.com/yarkable/RP_Infantry_Plus.git |
| `sp_vision_25_nonros` | https://github.com/unnc-aim/sp_vision_25_nonros.git |
| `breeze` | https://github.com/RobotPilots-SZU/breeze.git |

## Results

Clone root: `open_source_vision_repos/`

| Local directory | Status | Branch | Commit | Size |
| --- | --- | --- | --- | --- |
| `jlu_vision_26` | OK | `master` | `0dff17c` | 234M |
| `SHtech_auto_aim` | OK | `ax650-dev-2026` | `08af342` | 233M |
| `Climber_Vision_26` | OK | `main` | `9953692` | 47M |
| `sentry-auto-aim` | Failed | - | - | - |
| `TGU_Vision_2026` | OK | `main` | `9e7da0b` | 220M |
| `AT_NN_Detector` | OK | `main` | `d7ceb26` | 91M |
| `26-orin-Gimbal-AutoAim` | OK | `main` | `f8f70f2` | 62M |
| `robomaster-cv` | OK | `main` | `f186af5` | 57M |
| `AIRS-RM-2025` | OK | `main` | `16a69e2` | 7.9M |
| `RobotDetectionModel` | OK | `main` | `babaebd` | 17M |
| `RP_Infantry_Plus` | OK | `master` | `6075fbf` | 33M |
| `sp_vision_25_nonros` | OK | `main` | `cde577f` | 220M |
| `breeze` | OK | `main` | `4c71d91` | 5.3M |

All successful checkouts report a clean `git status`.

`robomaster-cv` was cloned with recursive submodules. Its `DeepStream-Yolo` and `darknet` submodules checked out successfully.

No Git LFS pointer files were found in the cloned repository files. The local environment also does not have `git lfs` installed.

`sentry-auto-aim` failed because Gitee returned an authentication prompt for `https://gitee.com/ustl-cod/sentry-auto-aim.git`; `git ls-remote` returned the same authentication failure with terminal prompts disabled.

Retest after the URL was confirmed manually:

- `git clone https://gitee.com/ustl-cod/sentry-auto-aim.git open_source_vision_repos/sentry-auto-aim` failed in this non-interactive environment with `could not read Username for 'https://gitee.com'`.
- `git ls-remote https://gitee.com/ustl-cod/sentry-auto-aim.git` failed with the same Gitee credential prompt.
- `git ls-remote https://git:git@gitee.com/ustl-cod/sentry-auto-aim.git` reached Gitee but failed with `Incorrect username or password (access token)`.
- The repository web page is publicly readable and reports default branch `master`, but Gitee's archive endpoint returned `reject by [gitee]`.

No partial `sentry-auto-aim` checkout was kept.

The current directory `/home/cr/Codes/autoaim` is not a git repository, so no outer commit was created.
