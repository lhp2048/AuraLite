# Task 6 Report: remove GDI+ backend from AuraLite master

**Status:** DONE  
**Commit:** pending `chore: remove GDI+ backend from AuraLite master`  
**Branch:** `master`  
**Base:** `f8f34f4` (Task 5)

## What implemented

Master self-draw path no longer uses GDI+. `gfx::Color` / `Brush` / `Bitmap` / `BitmapOperations` / `PlatformBitmapWin` are ARGB/BGRA + WIC. `CanvasD2D::FillRectInt(Brush)` reads `gfx::Brush` (solid / linear gradient). Window icons use `Bitmap::CreateHICON()` (DIB + `CreateIconIndirect`). Deleted `gfx/canvas_gdiplus.*` and `gfx/gdiplus_initializer.*`. Dropped `gdiplus.lib` from CMake / `AuraLite.UI` / `test_view` / `test_base`.

Allowed exceptions (not AuraLite canvas): native HWND (`NativeButton` / `NativeControlWin` / `NativeViewHost`) and Win32 `TrackPopupMenu`. `dumpbin` still shows `GDI32.dll` on `test_view` for those plus `CreateHICON`.

Also added previously untracked CMake / `auralite` / `d2d_demo` foundation so `cmake --build build --config Debug` is green from a clean tree.

## rg -i gdiplus

Business / build (cpp,h,cmake,txt,vcxproj,vcproj,ps1,py,props,sln) under `3rd-party/AuraLite`: **zero hits**.

Remaining hits are documentation only:

| Path | Why |
|---|---|
| `README.md` | States master backend is Direct2D-only; GDI+ removed; `gdiplus.lib` / `gdiplus.h` named as gone |
| `.superpowers/sdd/*` | Prior task briefs/reports |

`dumpbin /DEPENDENTS` on `test_view.exe` / `d2d_demo.exe` / `test_base.exe`: **no `gdiplus.dll`**. Present: `d2d1.dll`, `DWrite.dll`, `ole32.dll`; `test_view` also `GDI32.dll`.

## Build / run

```powershell
cd d:\Users\mx\Desktop\smart-family\family_win_desktop\3rd-party\AuraLite
cmake --build build --config Debug
.\bin\x64\Debug\test_base.exe
.\bin\x64\Debug\test_wic_bitmap.exe
.\bin\x64\Debug\test_view.exe
.\bin\x64\Debug\d2d_demo.exe
```

| Target | Result |
|---|---|
| AuraLite.Base | success `lib\x64\Debug\AuraLite.Base.lib` |
| AuraLite.UI | success `lib\x64\Debug\AuraLite.UI.lib` |
| auralite_d2d | success `lib\x64\Debug\auralite_d2d.lib` |
| test_view | success `bin\x64\Debug\test_view.exe` |
| d2d_demo | success `bin\x64\Debug\d2d_demo.exe` |
| test_base | success, exit 0 |
| test_wic_bitmap | `OK wic 2x2 DrawBitmapInt` exit 0 |

**test_view smoke:** pid stayed up 3s, `HasExited=False`, `MainWindowHandle` non-zero (title 测试视图). Stopped after smoke.  
**d2d_demo smoke:** pid stayed up 3s, `HasExited=False`, title `AuraLite D2D Demo`.

First full build hit `LNK1168` on `d2d_demo.exe` (file locked by a running instance). Killed the process and rebuilt; subsequent full Debug build was clean.

## Files changed (this commit)

| File | Change |
|---|---|
| `gfx/canvas_gdiplus.*` | deleted |
| `gfx/gdiplus_initializer.*` | deleted |
| `gfx/color.h` | ARGB `uint32`, no `gdiplus.h` |
| `gfx/brush.h` | solid / linear-gradient, no `Gdiplus::Brush` |
| `gfx/bitmap.*` / `platform_bitmap*` | BGRA buffer only; `CreateFromPixels`; `CreateHICON` |
| `gfx/bitmap_operations.cpp` | CPU blend on BGRA |
| `gfx/color_utils.cpp` | favicon / luma from `GetPixels` |
| `gfx/canvas.h` / `canvas_d2d.cpp` | drop `AsCanvasGdiplus`; D2D gradient from `gfx::Brush` |
| `view_framework/window/window_win.cpp` | `CreateHICON` |
| `test_view/main.cpp` | no `GdiplusInitializer` |
| CMake / vcxproj / scripts | no `gdiplus.lib` |
| `CMakeLists.txt` `cmake/` `auralite/` `examples/d2d_demo/` | tracked for green CMake |
| `README.md` | master backend Direct2D-only; native HWND / TrackPopupMenu exceptions |

## Concerns

1. **`GDI32.dll` remains** on `test_view` (native HWND, `CreateHICON`, `BeginPlatformPaint` HDC). Allowed; not GDI+.
2. **No ID2D1Bitmap cache** (Task 4 debt). Each `DrawBitmapInt` still uploads CPU pixels.
3. **Radio/Switch geometry** still axis-aligned rects, not ellipses.
4. **`.gitignore` rewritten** to CMake-oriented ignores (`/build/` `/bin/` `/lib/`). Old Eclipse/VS glob list dropped.
5. **One-shot scripts** (`scripts/fix_test_view_*.py`) left untracked; not needed to build.
