# Task 6 Report: ScrollView / ListView

**Status:** DONE  
**Date:** 2026-08-15  
**Branch:** `master`  
**Repo:** `family_win_desktop/3rd-party/AuraLite`

## Summary

Added vertical `ScrollView` (clip + thin scrollbar + wheel + thumb drag) and single-select `ListView` (string items, highlight, arrow keys). Extended `auralite::Canvas` with `PushAxisAlignedClip` / `PopAxisAlignedClip`. Window wheel dispatch walks from the hit node up to the first `WantsMouseWheel()` ancestor (`ScrollView`). `ui_smoke` embeds a 30-item `ListView` inside a 180px-tall `ScrollView`.

## Commits

| SHA | Subject |
|-----|---------|
| `f0e03d9` | `feat(ui): add ScrollView and ListView` |

Base: `e4b5f87` (Task 5 Checkbox / Radio / Switch).

## Files created / modified

**Created**
- `auralite/ui/scroll_view.h` / `scroll_view.cpp`
- `auralite/ui/list_view.h` / `list_view.cpp`

**Modified**
- `auralite/canvas.h` / `canvas_d2d.cpp` — clip push/pop
- `auralite/ui/node.h` — `WantsMouseWheel()`
- `auralite/ui/window.cpp` — wheel ancestor walk
- `CMakeLists.txt` — wire sources into `auralite_ui`
- `examples/ui_smoke/main.cpp` — tall scrollable list

**Not committed (per constraints)**
- `.superpowers/**` (including this report)

## Interfaces delivered

| Type | API |
|------|-----|
| `ScrollView` | `preferred_size` / `set_content` / `scroll_offset` / `OnMouseWheel` / thin scrollbar + thumb drag / clip paint |
| `ListView` | `AddItem` / `ClearItems` / `set_selected_index` / `on_selection_changed` / single-select highlight / Up/Down/Home/End |
| `Canvas` | `PushAxisAlignedClip` / `PopAxisAlignedClip` |
| `Window` | wheel → hit → walk parents → first `WantsMouseWheel()` |

## Build / test

```text
cmake --build build --config Debug --target ui_smoke
```

- **Build:** OK → `bin/x64/Debug/ui_smoke.exe`
- **Run:** process stayed alive ~2s with non-zero `MainWindowHandle`; stopped after smoke. Manual wheel/thumb/selection not fully automated.

## Self-review

**Matches brief**
- [x] ScrollView clip via Canvas Push/Pop
- [x] Wheel hit-chain to ScrollView
- [x] Thin scrollbar (drag included)
- [x] ListView single-select + inside ScrollView
- [x] `ui_smoke` tall scrollable list
- [x] CMake wired; commit message exact; no push; no `.superpowers` in commit

## Concerns

1. **ScrollView Measure** — content measured with `max_h = 1e6` so ListView reports full height; fine for phase-2 lists, not virtualized.
2. **ListView text width** — approximate (`font_size * 0.55 * len`); ScrollView stretches content to viewport width.
3. **No auto-scroll-into-view** when changing ListView selection with keyboard while scrolled away.
4. **Clip without depth counter** — caller must balance Push/Pop; mismatched calls would fault the D2D target.
