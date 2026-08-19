# Windows 7 特殊编译（备忘）

默认产物是 **Windows 10+**。本文不是主干功能，是日后若要 Win7 包时的做法。C++20 / VS2022 可不动。

用 **独立 CMake 开关、另一套产物**，不要在 Win10 路径里铺满运行时 `if (Win7)`。控件 / YAML / 主题 / D2D 绘制尽量不改。

## 目标（只做这两档）

1. **能启动、系统标题栏、表单**（低，约 1～2 天）
2. **无边框**：矩形窗 + 自绘 TitleBar 拖/关，系统标题栏不出现；可缩放。不做 DWM 圆角/阴影/贴边。

不做：每监视器 DPI、DWM 圆角、和 Win10 观感对齐、万能单 exe 探测降级。

## 开关

建议：`MXUI_WIN7=ON`

- `_WIN32_WINNT` / `WINVER` = `0x0601`
- **不要**链接 `shcore`（Win8 才有；链进去 Win7 可能缺 `SHCORE.dll` 起不来）
- 动态 CRT 在 Win7 上要 SP1 + 通用 CRT（KB2999226）+ 对应 VC 红包，且新红包对 7 不保证。Win7 包优先考虑 **`/MT`** 或单独工具集，这是装机问题，不是控件问题。

## 1. 能启动 + 系统标题栏 + 表单

| 改哪里 | 做什么 |
|--------|--------|
| `CMakeLists.txt` | 上述宏；`mx_ui` 去掉 `shcore` |
| `window.cpp`：`QueryHwndDpi` / `QueryMonitorDpiNearCursor` | `GetDpiForWindow` / `GetDpiForMonitor` 没有时用 `GetDeviceCaps(LOGPIXELSX)`，禁止回落写死 96 |
| `Application::EnableDpiAwareness` | 已有 `SetProcessDpiAwarenessContext` → `SetProcessDPIAware`，确认 Win7 走后者 |
| `Window::ApplyChromeDwm` | 整段跳过，或失败即忽略。不要依赖 `DWMWA_COLOR_NONE` / 圆角 / backdrop |
| `GetSystemMetricsForDpi` | 没有则 `GetSystemMetrics` |

系统标题栏窗（`caption: true`）应能用。分层菜单 / Toast（`UpdateLayeredWindow`）尽量保留。`WM_DPICHANGED` 在 7 上没有，全程系统 DPI。

验收：Win7 SP1 虚机；系统 96 与 125%；Aero 开/关；能开 gallery 或 login 系统边框窗、点按钮、打字。

## 2. 无边框（矩形、可缩放、无系统标题栏）

Win10 可缩放无边框是 `WS_CAPTION` + **DWM** `COLOR_NONE` 把系统标题藏掉。Win7 不用 DWM 藏标题：**根本不要系统非客户区标题**。

**要做：** 矩形无边框、`TitleBar` 拖/关、可缩放、客户区控件自绘。  
**不做：** DWM、窗口圆角、阴影、贴边（Aero Snap）。

| 做 | 不做 |
|----|------|
| `caption: false` → `WS_POPUP`，没有系统标题栏 | `WS_CAPTION` + `DWMWA_COLOR_NONE` |
| `TitleBar` 拖移、关闭（最小化按 YAML） | DWM 圆角、阴影、backdrop |
| `resizable` → `WS_THICKFRAME`，拖边缩放（方角） | `SetWindowRgn` 圆角、缩放时 DWM 玻璃 |
| 客户区 D2D 自绘与 Win10 相同 | 贴边、和 Win10 一样的最大化/任务栏细节 |
| 需要任务栏按钮时加 `WS_EX_APPWINDOW` | |

`ApplyChromeDwm` 整段跳过。不要 `SetWindowRgn`（圆角与可缩放冲突，Win7 目标就是矩形）。

验收：无系统标题栏；拖 TitleBar 能移动；点关能关；拖边能改大小；客户区按钮/输入能用。不验收圆角、阴影、贴边。

## 明确不做

- 每监视器 DPI
- Win11 DWM 圆角、亚克力 / backdrop
- 为 Win7 改控件绘制或 YAML
- 一项改动同时服务「单 exe 全系统」

## 复杂度

| 档 | 范围 | 量 |
|----|------|----|
| 低 | CMake + DPI 回落 + DWM 跳过 | 约 1～2 天 + 虚机冒烟 |
| 中 | 无边框 `WS_POPUP` + `THICKFRAME`：无系统标题、拖/关、可缩放 | 再 2～5 天 |

主文件：`CMakeLists.txt`、`mx/ui/application.cpp`、`mx/ui/window.cpp`（`Create` / `ApplyChromeDwm` / DPI 查询）。
