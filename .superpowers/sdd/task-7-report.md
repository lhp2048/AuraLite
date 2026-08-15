# Task 7 Report: SplitView / ContextMenu

**Status:** DONE  
**Date:** 2026-08-15  
**Branch:** `master`  
**Repo:** `family_win_desktop/3rd-party/AuraLite`

## Summary

Added horizontal `SplitView` (two panes + draggable divider, ratio clamp) and `ContextMenu` (`AddItem` / `AddSeparator` / `Show` via Win32 `TrackPopupMenu`). Wired `WM_CONTEXTMENU` in `Window` to the hit node’s `OnContextMenu` (optional handler, then parent walk). `ui_smoke` demos both: split panes + right-click menu (Refresh / About / Reset ratio).

## Commits

| SHA | Subject |
|-----|---------|
| `8737d36` | `feat(ui): add SplitView and ContextMenu` |

Base: `f0e03d9` (Task 6 ScrollView / ListView).

## Files created / modified

**Created**
- `auralite/ui/split_view.h` / `split_view.cpp`
- `auralite/ui/context_menu.h` / `context_menu.cpp`

**Modified**
- `auralite/ui/node.h` / `node.cpp` — `OnContextMenu` + `set_on_context_menu` + parent bubble
- `auralite/ui/window.h` / `window.cpp` — `WM_CONTEXTMENU` → hit / focused → `OnContextMenu`
- `CMakeLists.txt` — wire sources into `auralite_ui`
- `examples/ui_smoke/main.cpp` — SplitView + ContextMenu demo

**Not committed (per constraints)**
- `.superpowers/**` (including this report)

## Interfaces delivered

| Type | API |
|------|-----|
| `SplitView` | `preferred_size` / `set_leading` / `set_trailing` / `set_ratio` / drag divider (horizontal) |
| `ContextMenu` | `AddItem(id, label)` / `AddSeparator` / `on_command` / `Show(HWND, POINT\|xy)` → `TrackPopupMenu` |
| `Node` | `set_on_context_menu` / `OnContextMenu(screen_x,y)` (bubble to parent) |
| `Window` | `WM_CONTEXTMENU` dispatch (mouse + keyboard `-1`) |

## Build / test

```text
cmake --build build --config Debug --target ui_smoke
```

- **Build:** OK → `bin/x64/Debug/ui_smoke.exe`
- **Run:** process stayed alive ~2s; stopped after smoke. Manual drag / menu not fully automated.

## Self-review

**Matches brief**
- [x] SplitView horizontal two children + draggable splitter
- [x] ContextMenu AddItem / AddSeparator / Show via TrackPopupMenu
- [x] Window right-click → hit node OnContextMenu
- [x] `ui_smoke` demos both
- [x] CMake wired; commit message exact; no push; no `.superpowers` in commit

## Concerns

1. **Horizontal only** — vertical split deferred (spec allows “先做水平”).
2. **ContextMenu is not a Node** — helper wrapping system menu; fine per phase-2 exception.
3. **Parent bubble** — if hit has no handler, walks parents; root Column hosts the smoke menu.
4. **Ratio reset** — menu command calls `Layout(bounds())` directly; does not set Window `layout_dirty_`.

## Review (read-only)

**Verdict:** **APPROVE** (`f0e03d9` → `8737d36`)  
See `task-7-review.md` for checklist and notes. Full diff: `task-7-review-package.md` / `git diff f0e03d9..8737d36`.
