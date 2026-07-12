# 最新视觉模型仓库调研结果

日期：2026-07-12

## 输出文件

- `docs/arch/LATEST_MODEL_RESEARCH_PLAN_2026-07-12.md`
- `docs/arch/LATEST_MODEL_RESEARCH_2026-07-12.md`
- `docs/arch/LATEST_MODEL_RESEARCH_RESULTS_2026-07-12.md`

## 远端版本核验

使用 GitHub API 重新确认了 5 个纳入调研仓库的配置分支最新 commit：

| 仓库 | 分支 | commit | 日期 |
| --- | --- | --- | --- |
| `CCZU-Climber/Climber_Vision_26` | `main` | `99536929f0d0` | 2026-06-13 |
| `PraySky1337/AT_NN_Detector` | `main` | `d7ceb26cbec2` | 2026-06-11 |
| `broalantaps/RobotDetectionModel` | `main` | `babaebd6c8b3` | 2026-06-02 |
| `Fskaaaaaaaa/jlu_vision_26` | `master` | `0dff17cf0e88` | 2026-05-29 |
| `Astra-Whale/SHtech_auto_aim` | `ax650-dev-2026` | `08af342611c0` | 2026-05-03 |

`breeze` 更新日期更新，但没有视觉模型资产，因此从本次模型调研主体中排除。

## 调研内容

调研正文覆盖：

- 每个仓库的模型清单。
- 每个模型的输入、输出、任务和属性。
- 推理后端架构。
- 可运行设备和平台。
- 适用场景建议。
- 静态调研无法确认的缺口。

## 验证

- 工作区在写文档前为干净状态。
- 新增内容仅为 Markdown 文档。
- 后续提交前将运行 `git diff --check`。
