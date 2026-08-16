# AuraLite

Windows UI 工具库（源自早期 Chromium Views），作为第三方依赖放在 `family_win_desktop/3rd-party/AuraLite`。

上游仓库：https://github.com/lhp2048/AuraLite

**编码风格：** 见 [`CODING.md`](CODING.md)（`base` / `gfx` / `message_framework` / `view_framework` 以 Chromium 老 Views 习惯为准；`auralite/` 新栈见文中例外）。

## 分支策略

| 分支 | 用途 |
|------|------|
| **`1.x`** | 业务维护线：Chromium Views + GDI+、VS2022 静态库、C++14 / Win7+。Family Shell 等优先跟此分支。 |
| **`2.x`** | 过渡维护线：Chromium Views + Direct2D（阶段一完成态）。从 `master@acc1dab` 切出；Views/D2D 缺陷修复与兼容合入此线。 |
| **`master`** | 下一代开发线：**阶段二起**主做 `auralite::ui`（声明式 UI / YAML / 新控件树）。不以 `view::` 兼容为验收目标。 |

`1.x` 从 `534701a` 切出；`2.x` 从阶段一完成提交 `acc1dab` 切出。架构级新能力优先在 `master`，稳定后再视需要回港或另开大版本。

### 2.x / 阶段一遗产（**已完成** — Views + D2D）

**完成定义（2026-08-14）：** Views 自绘路径仅 Direct2D + DirectWrite + WIC；`CreateCanvas` / `WidgetWin` 回屏均为 D2D；**GDI+ 已移除**；`test_view` 与 `d2d_demo` 文字观感同一档。详见：

[`family_win_desktop/docs/superpowers/plans/2026-08-14-auralite-d2d-only-roadmap.md`](../../docs/superpowers/plans/2026-08-14-auralite-d2d-only-roadmap.md)

**已知例外（非 AuraLite 画布，仍走系统 GDI/HWND）：**

| 类型 | 说明 |
|------|------|
| `NativeButton` / `NativeControlWin` / `NativeViewHost` | 嵌原生 HWND，系统自绘 |
| `TrackPopupMenu`（`MenuModel` / `MenuRunner`） | Win32 系统弹出菜单 |

### master 阶段二（`auralite::ui` 声明式 UI）

在 `auralite::Canvas` 上新建控件树与声明式双轨（**YAML + C++ fluent DSL**），**不**依赖 `view::` / `AuraLite.UILegacy`。设计见：

[`family_win_desktop/docs/superpowers/specs/2026-08-15-auralite-phase2-declarative-ui-design.md`](../../docs/superpowers/specs/2026-08-15-auralite-phase2-declarative-ui-design.md)

| 目标 | 说明 |
|------|------|
| `auralite_d2d` (`AuraLite::D2D`) | `auralite::Canvas` / `Image`（D2D + DirectWrite + WIC） |
| `auralite_ui` (`AuraLite::UI`，产物 `AuraLite.UI.lib`) | `auralite::ui` 控件树 + `ViewFactory` + yaml-cpp + `dsl::*` + reactive/async |
| `login_demo` | 登录窗：默认读 `login_window.yaml`；`--fluent` 用链式等价树 |
| `ui_gallery` | 全控件面画廊（YAML 默认 / `--fluent`）；右键 `PopupHost` YAML 菜单 |
| `ui_smoke` | 早期冒烟（可选保留） |
| `AuraLite.Base` / `AuraLite.UILegacy` | 旧 Views 静态库（对照 / Shell 迁移前）；**新 Demo 不链接** |
| `d2d_demo` / `test_view` | 阶段一 / Views 冒烟 |

**与分支关系：** `1.x` = Views+GDI+；`2.x` = Views+D2D 冻结线；`master` = 阶段二 `auralite::ui` + **阶段三 reactive/async**。新 Demo **只**链 `AuraLite::UI`（传递依赖 D2D + yaml-cpp + `AuraLite::Base` / MessageLoop）。

### master 阶段三（异步与响应式）

设计 / 计划：

- [`…/specs/2026-08-15-auralite-phase3-reactive-async-design.md`](../../docs/superpowers/specs/2026-08-15-auralite-phase3-reactive-async-design.md)
- [`…/plans/2026-08-15-auralite-phase3-reactive-async.md`](../../docs/superpowers/plans/2026-08-15-auralite-phase3-reactive-async.md)

| 模块 | 说明 |
|------|------|
| `auralite::reactive` | `Signal` / `Computed` / `Observe` / `Batch`（读时追踪；**仅 UI 线程**，Debug assert） |
| `auralite::async` | `ResumeOnUi` / `Delay` / `RunAsync` / `SpawnUi`（挂 `MessageLoopForUI`）；可选传入 `Window::alive_flag()`，关窗后**不再 resume** |
| `auralite::ui` 绑定 | **单向** `BindText` / `BindVisible` / `BindEnabled`（Button/ImageButton）/ `BindChecked` / `BindValue` / `BindIndeterminate` / `BindItems`（VirtualList + ItemList）；写回用控件事件 |
| `Application::Run` | `MessageLoopForUI`（与 Family Shell 同一套循环）；`Window::Invalidate` 同 turn 合并为一次 PostTask |

**线程 / 生命周期：**

- `Signal` / `Computed` / `Observe`：**仅 UI 线程**（Debug assert）。
- 协程回 UI：`co_await Delay(ms, alive)` / `RunAsync(fn, alive)` / `ResumeOnUi(alive)`，其中 `alive = window.alive_flag()`。
- **MSVC：** 禁止用临时 lambda 启动协程（`SpawnUi([...]() -> FireAndForget { ... })` 易崩溃）；用自由函数或具名可调用对象。

**示例：**

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
Batch([&] { n.Set(2); /* other Signals… */ });

// Free function — do not use a temporary coroutine lambda on MSVC.
FireAndForget Load(std::shared_ptr<std::atomic_bool> alive) {
  co_await Delay(100, alive);
  co_await RunAsync([] { /* worker */ }, alive);
  // … update Signals on UI …
}
```

**Demo / 测试：** `reactive_test`、`async_test`（控制台）、`reactive_demo`（小验收）、`reactive_gallery`（控件面 + Signal/Bind 展示，对齐 `ui_gallery`）。

**非目标（本阶段不做）：** YAML 绑定表达式、自动双向绑定、`ObservableList` 细粒度 diff、Family Shell 迁移。

#### Mini YAML 子集（Spec §3.4）

- UTF-8 文件；UI 字符串宽字符  
- 2 空格缩进；`TypeName:` 作为唯一 map key（如 `Column:` / `Button:`）  
- 容器：`children:` 列表；`ScrollView.content`；`SplitView.leading` / `trailing`  
- 布局容器：`Column` / `Row`（流式）、`Tile`（网格，对应 DuiLib TileLayout）、`Tab`（叠页，对应 TabLayout）、`Absolute`（浮动 / 四边锚定，对应 float）  
- `on_click: handler_name` → Demo 侧 `HandlerMap`  
- 尺寸策略：`width` / `height` 可为数字（Fixed）、`fill`（吃满父布局可用轴）、`hug`（内容固有尺寸）。省略时控件有默认（如 TextField/Button：**宽 fill、高 fixed**；Checkbox：**hug×hug**；Label：**宽 fill、高 hug/preferred_height**）  
- `Column` / `Row`：仅主轴 **`fill`** 的子项参与 `weight` 分剩余（Fixed/Hug 上的 weight 忽略）；`cross_align` / `child_align`；无 fill 子项时可用 `main_align` 打包（start|center|end）。Label 的 `align` 仍是**文本**对齐  
- 横排工具按钮请显式 `width: hug`（Button 默认宽 fill，适合表单）  
- `Absolute`：锚定优先 `left`/`top`/`right`/`bottom`；否则 `x`/`y` + 自有宽高  
- `Tab`：可选 `headers` / `header_height` 页签栏；`selected`  
- `Tile`：`columns` / `item_size` / `spacing`  
- 新增控件：`ProgressBar`（`value` / `indeterminate`，不确定态需 `BindWindow`）、`Slider`（`orientation` / `step` / `tick_count`）、`Combo`（单选/多选 `multi`、可筛选 `editable`，需 `BindWindow`）、`TextArea`（多行，`wrap` 软换行）、`VirtualList` / `ItemList`（`columns` + `show_header`；排序 / 拖列宽 / `frozen_count`；Shift+滚轮横滑）、`TreeView`（展开折叠；`checkable` 三态勾选；`lazy` + `on_load_children` / `NotifyChildrenLoaded`）
- **富文本**：本阶段不做，业务可自行集成
- 无热重载、无完整 schema
- **弹出菜单（推荐）：** `PopupHost` + `Window::CreatePopup` 承载任意 YAML/DSL 控件树；`Submenu` 为薄触发行，通过 `PopupHost::Push` 叠层。Esc 先关最上层，再关根层；点菜单外关闭整栈。项回调可用 `WrapDismiss` 在执行后 `Dismiss()`。弹出层为分层窗口、逐像素 alpha：未绘制区域透明，可透出宿主窗口；面板填充可在根 `Column`/`Row` 等节点设 `bg: "#RRGGBBAA"`（8 位含 alpha），纯浮动控件可省略 `bg`。Gallery 右键默认 `menu_classic.yaml`（扁平菜单行）；`popup_menu.yaml` 仍为任意控件树示例。
- **Legacy：** `ContextMenu` 仍为代码 API（`TrackPopupMenu`），暂不删除；新代码请用 `PopupHost`

#### 与 DuiLib 对照（布局）

| DuiLib | AuraLite |
|--------|----------|
| Vertical / HorizontalLayout | `Column` / `Row` + weight + cross/main_align |
| TileLayout | `Tile` |
| TabLayout | `Tab`（可带 `headers`） |
| float + 边距 | `Absolute` + 四边锚定 / `x`·`y` |
| 同容器混 float | 浮动子项放进 `Absolute`（不在 Column 内混排） |

布局单测：`ui_layout_test`（`cmake --build ... --target ui_layout_test` 后运行 exe）。

#### 阶段二入口

```powershell
cd family_win_desktop\3rd-party\AuraLite
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug --target login_demo ui_gallery auralite_ui ui_layout_test
.\bin\x64\Debug\login_demo.exe
.\bin\x64\Debug\login_demo.exe --fluent
.\bin\x64\Debug\ui_gallery.exe
.\bin\x64\Debug\ui_gallery.exe --fluent
.\bin\x64\Debug\ui_layout_test.exe
```

产物目录：`bin|lib/<Platform>/<Config>/`（YAML 会复制到 exe 旁）。

#### Theme

- 全局 `Theme::Active()` 提供颜色 / 字体 token；内置 `light` / `dark`，也可从 `themes/*.yaml` 注册扩展主题
- 运行时切换：`Theme::SetActive("dark")`（会 `Invalidate` 已绑定窗口）
- 同名 `Register` / `RegisterFromFile` 若正是当前主题，会刷新 Active 并通知窗口
- 控件未调用 `font_size(...)` 时回落 `fonts.size`（`ResolveFontSize`）；颜色等同理可稀疏覆盖
- 控制台冒烟：`theme_test`（`examples/theme_test`）

`ui_gallery` 示例：`examples/ui_gallery/themes/` 下 YAML 与 Light/Dark 按钮（手测换肤）；底部 **Popup 菜单样式** 三个按钮试开 `menu_classic` / `popup_menu`（按钮风）/ `menu_dark`；空白处右键默认经典菜单。

#### PopupHost / 自绘弹出菜单

弹出窗口使用 `WS_EX_LAYERED` + `UpdateLayeredWindow`：**未绘制像素完全透明**，菜单间隙可看到下层窗口；不在客户区铺不透明底色。

| API | 说明 |
|-----|------|
| `PopupHost::Show` / `ShowFromYaml` | 根层弹出；内容为任意 `Node` 树 |
| `Window::CreatePopup` | 分层 `WS_POPUP` 承载层（由 host 使用） |
| `Submenu` | 菜单行；hover/click → `Push` 子层；dismiss 后 content 归还 |
| `WrapDismiss` | 包装 `on_click`：执行后关闭整栈 |
| `ContextMenu` | **Legacy** `TrackPopupMenu`；勿作为新菜单路径 |

**面板背景（YAML）：** 在根容器或子菜单 `content` 的 `Column`/`Row` 上设 `bg: "#RRGGBBAA"`（如 `#F5F7FAE6` 半透明白）绘制填充；省略 `bg` 时仅绘制子控件（适合纯浮动按钮）。子菜单第二层与根层行为相同。经典菜单样式见 `examples/ui_gallery/menu_classic.yaml`（Button：`bg`/`bg_hover`/`text_align`/`corner_radius`）；控件树示例见 `popup_menu.yaml`。

关闭 Views：默认 **不** 编 `AuraLite.UILegacy`（`-DAURALITE_BUILD_UILEGACY=OFF`）。`AuraLite.Base`（MessageLoop）仍默认编。旧别名 `-DAURALITE_BUILD_LEGACY=ON` 会打开 Views；`OFF` 只关 Views，不关 Base。旧 `library.sln` 仍可并行使用。

**新代码禁止** `#include` `view_framework`、禁止链接 `AuraLite.UILegacy.lib`。只链 `AuraLite::UI` + `AuraLite::Base`。

`auralite::ui` 布局/命中/绘制使用 **DIP**（96 DIP = 1 逻辑英寸）。`Window::Create(w,h)` 的宽高是 DIP；Per-Monitor V2 下 `WM_DPICHANGED` 会重布局。建窗前调用 `Application::EnableDpiAwareness()`（或等价 `SetProcessDpiAwarenessContext`）。

## 工程结构

源码目录仍按模块划分；**CMake 对外库名：**

| 工程 | 聚合内容 | 产物 |
|------|----------|------|
| **AuraLite.Base** | `base` + `rfc_algorithm` + `message_framework` | `AuraLite.Base.lib` |
| **AuraLite.UI**（`auralite_ui`） | `auralite::ui` + reactive/async（链 D2D） | `AuraLite.UI.lib` |
| **AuraLite.UILegacy** | `gfx` + `animation` + `view_framework` | `AuraLite.UILegacy.lib` |
| `test_base` | Base 冒烟（控制台） | `test_base.exe` |
| `test_view` | 旧 Views 示例（窗口） | `test_view.exe` |

### 基础控件（`view_framework/controls/`）

| 控件 | 说明 |
|------|------|
| `Button` / `TextButton` / `ImageButton` | 按钮（原有） |
| **`Label`** | 文本标签 |
| **`Textfield`** | 单行输入；`STYLE_PASSWORD` 密码模式 |
| **`Checkbox`** | 勾选框 + 文案 |
| **`RadioButton`** | 单选；同 `group_id` 互斥 |
| **`Switch`** | 开/关滑块 |
| **`ImageView`** | 图片展示（不可点） |
| **`ScrollView`** | 纵向滚动（滚轮命中鼠标下控件 + 细滚动条） |
| **`ListView`** | 单选列表项（可放进 ScrollView） |
| **`MenuModel` / `MenuRunner`** | 弹出菜单（Win32 TrackPopupMenu） |
| **`SimpleMenuModelController`** | 挂到任意 View 的右键菜单 |
| `NativeViewHost` / `NativeControlWin` | 嵌原生 HWND |

`Textfield` 默认自带编辑右键菜单（剪切 / 复制 / 粘贴 / 全选）。

布局：`BoxLayout` / `GridLayout` / `FillLayout`、`SingleSplitView`。

### master 渲染后端

**仅 Direct2D + DirectWrite + WIC**。GDI+ 已从 master 自绘路径移除（无 `gdiplus.lib`、无 `canvas_gdiplus`、Paint 路径无 `#include <gdiplus.h>`）。

允许例外（非 AuraLite 画布）：原生 HWND 控件（`NativeButton` / `NativeControlWin` / `NativeViewHost`）与系统 `TrackPopupMenu`。

### D2D 自绘控件验收

`test_view` 与 `d2d_demo` 同为 Direct2D + DirectWrite。控件 `Paint()` 只走 `gfx::Canvas` / `Font::GetStringWidth`。

| 控件 | 状态 | 说明 |
|------|------|------|
| Label / 标题色 | pass | DirectWrite；顶部含后端对照说明（窄宽时标题尾部裁剪，无省略号） |
| Textfield 文本、选区、插入符、密码圆点 | pass | 测宽 `Font::GetStringWidth`；密码为圆点 |
| Checkbox / RadioButton / Switch | pass | Checkbox 勾线；Radio 椭圆；Switch 圆角轨道 + 圆形滑块 |
| ListView 选中高亮 + ScrollView 滚动条与裁剪 | pass | ListView 放入 ScrollView；第 3 项预选蓝底；列表项被视口裁剪 |
| TextButton / ImageButton（含对齐） | pass | 左/中/右对齐；hover 经 D2D `PushLayer` 透明度 |
| SingleSplitView 分隔与背景 | pass | 标准面板渐变 + 实线边框 |
| 面板 `Background` / `Border` | pass | `FillRectInt` / 垂直渐变画刷 |

例外（非 AuraLite 画布）：`NativeButton` / `NativeControlWin`、系统 `TrackPopupMenu`。

解决方案：`library.sln`

```
AuraLite/
  AuraLite.Base/          # Base 静态库工程
  AuraLite.UILegacy/      # 旧 Views 静态库工程（产物 AuraLite.UILegacy.lib）
  AuraLite.Common.props   # 公共编译选项
  auralite/               # 新栈：ui / reactive / async / canvas（CMake → AuraLite.UI.lib）
  auralite_export.h
  base/  rfc_algorithm/  message_framework/
  gfx/   animation/      view_framework/
  test_base/  test_view/
  lib/<Platform>/<Configuration>/   # *.lib
  bin/<Platform>/<Configuration>/   # *.exe
```

`base/`、`gfx/` 等目录下的旧 `.vcxproj` 仅作历史参考。CMake：`AuraLite::UI`（新）/ `AuraLite::UILegacy`（Views）/ `AuraLite::Base`。

## 编译环境

| 项 | 取值 |
|----|------|
| IDE / 工具集 | VS 2022，`v143` |
| 平台 | **Win32** + **x64** |
| 配置 | Debug / Release |
| CRT | `/MD`（Debug 为 `/MDd`） |
| 语言 | MSVC `/std:c++14`（无 `/std:c++11`；C++11 源码可编） |
| 系统目标 | Win7+（`WINVER` / `_WIN32_WINNT` = `0x0601`） |
| 库形态 | **静态库**（不产出 DLL） |

已验证：`Debug|x64`、`Release|x64`、`Debug|Win32`、`Release|Win32`。

## 构建

```powershell
cd family_win_desktop\3rd-party\AuraLite
msbuild library.sln /p:Configuration=Debug /p:Platform=x64 /m
```

只编库（不编测试）：

```powershell
msbuild AuraLite.Base\AuraLite.Base.vcxproj /p:Configuration=Debug /p:Platform=x64
msbuild AuraLite.UILegacy\AuraLite.UILegacy.vcxproj /p:Configuration=Debug /p:Platform=x64
```

输出：

- 库：`lib\<Platform>\<Configuration>\AuraLite.Base.lib`、`AuraLite.UILegacy.lib`（CMake 新栈产物为 `AuraLite.UI.lib`）
- 测试：`bin\<Platform>\<Configuration>\test_base.exe`、`test_view.exe`

## 运行测试

```powershell
.\bin\x64\Debug\test_base.exe    # 控制台，消息循环冒烟，正常 exit 0
.\bin\x64\Debug\test_view.exe    # 弹出示例窗口
```

## 接入 FamilyShell

1. 头文件搜索路径加：`3rd-party\AuraLite`
2. **不要**链接 `AuraLite.UILegacy.lib`。新 UI：`AuraLite.UI.lib` + `AuraLite.Base.lib` + `auralite_d2d.lib`（及 `d2d1`、`dwrite`、`windowscodecs`、`imm32`、`shcore` 等）
3. 预处理器建议定义：`AURALITE_STATIC`、`NOMINMAX`、`_WIN32_WINNT=0x0A00`（新栈）

## 依赖说明

- **默认不依赖 ATL / MFC**。无障碍相关使用 MSAA stub（`view_accessibility_msaa.cpp`）。
- 若本机安装了 VC ATL/MFC，可定义 `AURALITE_HAS_ATL` 并恢复完整 ATL 无障碍实现（可选）。
- UI 绘制基于 **Direct2D / DirectWrite / WIC**（见 `gfx/`）。master 已移除 GDI+ 自绘后端。
