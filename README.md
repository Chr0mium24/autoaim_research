# autoaim_research

Research archive for RoboMaster vision and auto-aim repositories.

## Repository Layout

- `docs/arch/` contains architecture reports and selected copied source, config, model, and asset files used for offline review.
- `clone_reports/` records repository discovery and clone results.
- `open_source_vision_repos/` contains full upstream repositories as Git submodules pinned to the reviewed commits.

## Clone

```bash
git clone --recurse-submodules https://github.com/Chr0mium24/autoaim_research.git
```

If the repository has already been cloned:

```bash
git submodule update --init --recursive
```

One submodule, `open_source_vision_repos/sentry-auto-aim`, points to Gitee. If that host prompts for credentials, see `clone_reports/vision_repos_2026-07-09.md` for the original clone notes.

## Notes

This repository keeps copied research snapshots in `docs/arch/` and also tracks the corresponding upstream projects as submodules. Third-party code, models, and assets remain subject to their original upstream licenses and notices.
