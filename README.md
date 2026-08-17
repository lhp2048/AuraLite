# AuraLite

Windows 声明式 UI 库。本文描述 **`master`**：`auralite::ui` 控件树 + Direct2D 画布 + YAML / C++ fluent 双轨。

Views 时代（GDI+ / D2D）在 **`1.x`** / **`2.x`**，不是本分支的用法。

上游：https://github.com/lhp2048/AuraLite

| 文档 | 内容 |
|------|------|
| [`LAYOUT.md`](LAYOUT.md) | Column / Row / Absolute、`h_align` / `v_align`、锚点、Fill/Hug |
| [`CODING.md`](CODING.md) | 编码风格（`auralite/` 新栈 vs legacy） |

## 这是什么

- **控件树**在 `auralite::Canvas` 上自绘（Direct2D + DirectWrite + WIC），不走 `view::`。
- **YAML 与 C++ fluent DSL** 共用同一套布局和属性；`ui_gallery --check` 要求两棵树 Dump 对齐。
- **单位是 DIP**（96 DIP = 1 逻辑英寸）。`Window::Create(w, h)` 的宽高是 DIP；Per-Monitor V2 下 `WM_DPICHANGED` 会重布局。
- **主题**是进程级契约：`Theme::Active()` / `Theme::SetActive("dark")`。
- 新代码 **C++20**，只链 `AuraLite::UI`（传递 `AuraLite::D2D`、yaml-cpp、`AuraLite::Base` / MessageLoop）。

默认 **不编译** `AuraLite.UILegacy`。禁止 `#include` `view_framework`、禁止链接 `AuraLite.UILegacy.lib`。

## 快速开始

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug --target ui_gallery ui_layout_test auralite_ui
.\bin\x64\Debug\ui_gallery.exe
.\bin\x64\Debug\ui_gallery.exe --fluent
.\bin\x64\Debug\ui_gallery.exe --check
.\bin\x64\Debug\ui_layout_test.exe
```

产物：`bin|lib/<Platform>/<Config>/`。YAML 会复制到 exe 旁。

建窗前调用 `Application::EnableDpiAwareness()`（或等价 `SetProcessDpiAwarenessContext`）。

**对话框按键：** 模态 `RunModal` 时 **Esc** 为 `EndModal(IDCANCEL)`。`Button` 设 `default: true`（C++ `is_default(true)`）后，**Enter** 在焦点不在 `TextArea` / 当前按钮 / 打开的 `Combo` 时点这个默认按钮（焦点在别的按钮上则点那个按钮）。禁用的 default 会被跳过。

**快捷键表：** `Window::AddAccelerator("Ctrl+S", ...)` 或按钮 `accelerator: "F1"` / `.accelerator("Ctrl+S")`。只接受带 Ctrl/Alt 的组合、F1–F24、Esc；普通字母不会抢走输入。后登记的窗口快捷键优先于按钮。`HandleKey` 供测试或嵌入调用。

## 工程

| 目标 | 说明 | 产物 |
|------|------|------|
| `auralite_d2d` (`AuraLite::D2D`) | `auralite::Canvas` / `Image` | `auralite_d2d.lib` |
| `auralite_ui` (`AuraLite::UI`) | 控件树 + YAML + DSL + reactive/async | `AuraLite.UI.lib` |
| `AuraLite.Base` | MessageLoop（新栈依赖） | `AuraLite.Base.lib` |
| `AuraLite.UILegacy` | 冻结的 Chromium Views，默认关闭 | `AuraLite.UILegacy.lib` |

CMake 开关：

- `AURALITE_BUILD_BASE` 默认 ON（MessageLoop）
- `AURALITE_BUILD_UILEGACY` 默认 OFF
- 旧别名 `AURALITE_BUILD_LEGACY=ON` 会打开 Views；`OFF` 只关 Views，不关 Base

## YAML 子集

- UTF-8；UI 字符串宽字符；2 空格缩进
- `TypeName:` 作为唯一 map key（如 `Column:` / `Button:`）
- 容器：`children:`；`ScrollView.content`；`SplitView.leading` / `trailing`
- 根级可选 `theme: dark`（加载时 `Theme::SetActive`）
- 根级可选 `window:`（HWND 元数据，不进控件树）：`title` / `width` / `height` / `kind`（`main` \| `dialog` \| `popup`）/ `corner_radius` / `border_width` / `resizable` / `min_width` / `min_height` / `topmost` / `center_on_owner`
- `on_click: handler_name` → 应用侧 `HandlerMap`
- 布局细则见 [`LAYOUT.md`](LAYOUT.md)

```yaml
Column:
  width: fill
  height: fill
  h_align: center
  v_align: center
  children:
    - Button: { text: "确定", width: hug }
```

C++ 等价：

```cpp
using namespace auralite::ui::dsl;
auto root = Column()
    .fill_width()
    .fill_height()
    .h_align(Align::Center)
    .v_align(Align::Center)
    .child(Button().text(L"确定").hug_width())
    .Build();
```

### 尺寸与对齐（摘要）

- `width` / `height`：数字（Fixed DIP）、`fill`、`hug`。省略时看控件默认（Button / TextField：**宽 fill、高 fixed**；Checkbox：**hug×hug**；Label：**宽 fill、高 hug**）。
- 横排工具按钮请显式 `width: hug`。
- `Column` / `Row`：仅主轴 **fill** 子项参与 `weight`。`h_align` / `v_align` 按屏幕轴（左右 / 上下），不是 main/cross。Label 的 `align` 仍是**文本**对齐。
- `Absolute`：每轴 **双边锚点 > 单边 > `x`/`y` > `h_align`/`v_align` > 0**。左+右忽略该轴 `width`。

不做百分比尺寸、不做 `elevation` / `z-index`。叠层 = `children` 声明顺序。半透明只用 `bg: "#RRGGBBAA"`，没有控件级 `opacity`。

### 与 DuiLib 对照

| DuiLib | AuraLite |
|--------|----------|
| Vertical / HorizontalLayout | `Column` / `Row` + weight + h_align / v_align |
| TileLayout | `Tile` |
| TabLayout | `Tab`（可带 `headers`） |
| float + 边距 | `Absolute` + 四边锚定 / `x`·`y` |
| 同容器混 float | 浮动子项放进 `Absolute`（不要在 Column 里写 `left`） |

## 控件

| 类型 | 说明 |
|------|------|
| `Column` / `Row` / `Tile` / `Tab` / `Absolute` / `SplitView` / `ScrollView` | 布局 |
| `TitleBar` | 无 `children`：`[icon?][title][min][max/restore][close]`（min/max/close 默认开；无 `icon` 路径则省略图标）。有 `children`：列表即布局。`name` 覆盖 `icon` / `title` / `minimize` / `maximize` / `close` 槽参数。空白 / Label / 图标可拖窗 |
| `Label` | 默认单行 `trim: clip`；`wrap: true` 软换行；单行可 `trim: start\|middle\|end` 省略号 |
| `TextField` / `TextArea` | 单行 / 多行（`wrap` 软换行） |
| `Checkbox` / `Radio` / `Switch` | 选择 |
| `ProgressBar` / `Slider` / `Combo` | 进度、滑块、下拉（Combo 需 `BindWindow`） |
| `VirtualList` / `ItemList` / `ListView` / `TreeView` | 列表与树 |
| `UserControl` | 自绘扩展 |
| `PopupHost` / `Submenu` | 自绘弹出层（推荐） |
| `Toast` | 轻提示 |
| `ContextMenu` | Legacy `TrackPopupMenu`；新菜单用 `PopupHost` |

`window.title` 只用于任务栏。无边框窗可见标题用 `TitleBar`：

```yaml
TitleBar: { title: App, icon: app.png }
```

要加新按钮时把要用的默认槽也写进 `children`（顺序即布局）。只写一个新按钮时，标题栏就只有那一个按钮：

```yaml
TitleBar:
  title: Dialog
  children:
    - Label: { name: title }
    - Button: { text: "?", width: 32 }
    - Button: { name: close, text: "关" }
```

`name: close` 覆盖默认关闭键（glyph / 尺寸 / `Close()`）；未写进 `children` 的槽不会出现。最大化在还原态显示 restore glyph。DSL：`TitleBar().title(L"...").icon(L"app.png")`。

无边框窗（`caption: false`）默认可拖边 / 角缩放（约 6 DIP，光标随边变化）。`kind: dialog` / `WindowOptions::Dialog()` 默认 `resizable: false`。最大化时关掉。边上的 Button 优先于缩放。`window.resizable` / `min_width` / `min_height` 可配。

**本分支不做：** 富文本、YAML 热重载、完整 schema、百分比尺寸、控件级 opacity、项级 UIA pattern。

## 主题

- `Theme::Active()` 提供颜色 / 字体 token；内置 `light` / `dark`，也可从 YAML 注册
- `Theme::SetActive("dark")` 会 Invalidate 已绑定窗口
- 控件未设 `font_size` 时回落 `fonts.size`；颜色可稀疏覆盖
- Gallery：`examples/ui_gallery/themes/`

## 弹出层

`PopupHost` + `Window::CreatePopup`：分层窗口、逐像素 alpha，未绘制区域透明。

| API | 说明 |
|-----|------|
| `PopupHost::Show` / `ShowFromYaml` | 根层弹出 |
| `Submenu` | hover/click → `Push` 子层 |
| `WrapDismiss` | 包装 `on_click`：执行后关整栈 |

面板背景在根 `Column`/`Row` 上设 `bg: "#RRGGBBAA"`。Gallery 右键默认 `menu_classic.yaml`。

## 响应式与异步

| 模块 | 说明 |
|------|------|
| `auralite::reactive` | `Signal` / `Computed` / `Observe` / `Batch`（**仅 UI 线程**，Debug assert） |
| `auralite::async` | `ResumeOnUi` / `Delay` / `RunAsync` / `SpawnUi`（挂 `MessageLoopForUI`） |
| 绑定 | 单向 `BindText` / `BindVisible` / `BindEnabled` / `BindChecked` / `BindValue` / `BindItems`；写回用控件事件 |

关窗后不再 resume：把 `Window::alive_flag()` 传给 `Delay` / `RunAsync`。**MSVC 禁止用临时 lambda 启动协程**，用自由函数或具名可调用对象。

```cpp
using namespace auralite::reactive;
using namespace auralite::ui;
using namespace auralite::async;

Signal<int> n{0};
Computed<std::wstring> text{[&] {
  return L"n=" + std::to_wstring(n.Get());
}};
label->OwnSubscription(BindText(*label, text));
button->on_click([&] { n.Set(n.Peek() + 1); });
```

不做：YAML 绑定表达式、自动双向绑定、`ObservableList` 细粒度 diff。

## 无障碍 / 动画

- MSAA stub + UIA provider（控件级角色与名字；`acc_name` / tooltip）
- `anim`：属性动画；控件 hover 等可走主题过渡
- Tooltip 由窗口托管 overlay

## Demo / 测试

| 目标 | 用途 |
|------|------|
| `ui_gallery` | 全控件面（YAML 默认 / `--fluent` / `--check`） |
| `login_demo` | 登录窗；`--fluent` 为链式等价树 |
| `ui_layout_test` | 布局单测 |
| `theme_test` / `dpi_test` / `dialog_test` | 主题、DPI、模态对话框 |
| `reactive_test` / `async_test` / `reactive_demo` / `reactive_gallery` | Signal / 协程 |
| `acc_test` / `uia_test` / `toast_test` / `anim_test` / `drag_test` | 无障碍、Toast、动画、拖放 |

```powershell
cmake --build build --config Debug --target ui_gallery ui_layout_test theme_test
.\bin\x64\Debug\ui_layout_test.exe
.\bin\x64\Debug\theme_test.exe
```

## 目录

```
AuraLite/
  auralite/           # 新栈：ui / reactive / async / canvas
  examples/           # gallery、login、各类 *_test
  cmake/
  LAYOUT.md  README.md  CODING.md
  base/  message_framework/   # AuraLite.Base
  gfx/  animation/  view_framework/   # UILegacy，默认不编
  lib/<Platform>/<Config>/
  bin/<Platform>/<Config>/
```

## 编译环境

| 项 | 取值 |
|----|------|
| 生成器 | VS 2022，`v143` |
| 平台 | x64（亦可 Win32） |
| 配置 | Debug / Release |
| CRT | `/MD`（Debug `/MDd`） |
| 新栈语言 | C++20（`AuraLite::UI`） |
| Base / UILegacy | C++14 |
| 新栈系统宏 | `WINVER` / `_WIN32_WINNT` = `0x0A00` |
| 库形态 | 静态库 |

接入应用：头文件搜索路径加本仓库根目录；链接 `AuraLite.UI.lib` + `AuraLite.Base.lib` + `auralite_d2d.lib`，以及 `d2d1`、`dwrite`、`windowscodecs`、`imm32`、`shcore`、`UIAutomationCore` 等。预处理器：`AURALITE_STATIC`、`NOMINMAX`、`_WIN32_WINNT=0x0A00`。
