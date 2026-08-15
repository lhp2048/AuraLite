# AuraLite

Windows UI 工具库（源自早期 Chromium Views），作为第三方依赖放在 `family_win_desktop/3rd-party/AuraLite`。

上游仓库：https://github.com/lhp2048/AuraLite

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

在 `auralite::Canvas` 上新建控件树与声明式双轨（**YAML + C++ fluent DSL**），**不**依赖 `view::` / `AuraLite.UI`。设计见：

[`family_win_desktop/docs/superpowers/specs/2026-08-15-auralite-phase2-declarative-ui-design.md`](../../docs/superpowers/specs/2026-08-15-auralite-phase2-declarative-ui-design.md)

| 目标 | 说明 |
|------|------|
| `auralite_d2d` (`AuraLite::D2D`) | `auralite::Canvas` / `Image`（D2D + DirectWrite + WIC） |
| `auralite_ui` (`AuraLite::UINext`) | `auralite::ui` 控件树 + `ViewFactory` + yaml-cpp + `dsl::*` |
| `login_demo` | 登录窗：默认读 `login_window.yaml`；`--fluent` 用链式等价树 |
| `ui_gallery` | 全控件面画廊（YAML 默认 / `--fluent`）；含 ContextMenu |
| `ui_smoke` | 早期冒烟（可选保留） |
| `AuraLite.Base` / `AuraLite.UI` | 旧 Views 静态库（对照 / `1.x`·`2.x`）；**新 Demo 不链接** |
| `d2d_demo` / `test_view` | 阶段一 / Views 冒烟 |

**与分支关系：** `1.x` = Views+GDI+；`2.x` = Views+D2D 冻结线；`master` = 阶段二 `auralite::ui`。阶段二 Demo **只**链 `AuraLite::UINext`（传递依赖 D2D + yaml-cpp）。

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
- 无热重载、无完整 schema；`ContextMenu` 仍为代码 API（`TrackPopupMenu`）

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

关闭旧库：`-DAURALITE_BUILD_LEGACY=OFF`。旧 `library.sln` 仍可并行使用。

## 工程结构

源码目录仍按模块划分；**对外只编两个静态库**：

| 工程 | 聚合内容 | 产物 |
|------|----------|------|
| **AuraLite.Base** | `base` + `rfc_algorithm` + `message_framework` | `AuraLite.Base.lib` |
| **AuraLite.UI** | `gfx` + `animation` + `view_framework` | `AuraLite.UI.lib` |
| `test_base` | Base 冒烟（控制台） | `test_base.exe` |
| `test_view` | UI 示例（窗口） | `test_view.exe` |

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
  AuraLite.Base/          # 静态库工程
  AuraLite.UI/            # 静态库工程（依赖 Base）
  AuraLite.Common.props   # 公共编译选项
  auralite_export.h
  base/  rfc_algorithm/  message_framework/
  gfx/   animation/      view_framework/
  test_base/  test_view/
  lib/<Platform>/<Configuration>/   # *.lib
  bin/<Platform>/<Configuration>/   # *.exe
```

`base/`、`gfx/` 等目录下的旧 `.vcxproj` 仅作历史参考，请用 `AuraLite.Base` / `AuraLite.UI`。

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
msbuild AuraLite.UI\AuraLite.UI.vcxproj /p:Configuration=Debug /p:Platform=x64
```

输出：

- 库：`lib\<Platform>\<Configuration>\AuraLite.Base.lib`、`AuraLite.UI.lib`
- 测试：`bin\<Platform>\<Configuration>\test_base.exe`、`test_view.exe`

## 运行测试

```powershell
.\bin\x64\Debug\test_base.exe    # 控制台，消息循环冒烟，正常 exit 0
.\bin\x64\Debug\test_view.exe    # 弹出示例窗口
```

## 接入 FamilyShell

1. 头文件搜索路径加：`3rd-party\AuraLite`
2. 链接：`AuraLite.UI.lib` + `AuraLite.Base.lib`（及系统库：`d2d1`、`dwrite`、`windowscodecs`、`ole32`、`oleacc`、`dwmapi`、`uxtheme` 等，参见 `test_view`）
3. 预处理器建议定义：`AURALITE_STATIC`、`NOMINMAX`、`_WIN32_WINNT=0x0601`

## 依赖说明

- **默认不依赖 ATL / MFC**。无障碍相关使用 MSAA stub（`view_accessibility_msaa.cpp`）。
- 若本机安装了 VC ATL/MFC，可定义 `AURALITE_HAS_ATL` 并恢复完整 ATL 无障碍实现（可选）。
- UI 绘制基于 **Direct2D / DirectWrite / WIC**（见 `gfx/`）。master 已移除 GDI+ 自绘后端。
