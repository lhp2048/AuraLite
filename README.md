# AuraLite

Windows UI 工具库（源自早期 Chromium Views），作为第三方依赖放在 `family_win_desktop/3rd-party/AuraLite`。

上游仓库：https://github.com/lhp2048/AuraLite

## 分支策略

| 分支 | 用途 |
|------|------|
| **`1.x`** | 当前方案维护线：Chromium Views + GDI+、VS2022 静态库、C++14 / Win7+。Family Shell 等业务优先跟此分支。 |
| **`master`** | 下一代迭代线：按《AuraLite 项目开发计划书》推进（Direct2D、CMake、声明式 UI 等）。 |

`1.x` 从提交 `534701a`（基础控件与菜单就绪）切出；其后 bugfix / 小功能合入 `1.x`，架构级改造在 `master`。

### master 阶段一（已收口 → **范围已修订**）

> 旧阶段一（仅 Canvas + Demo）已完成；**新完成定义**见：  
> `family_win_desktop/docs/superpowers/plans/2026-08-14-auralite-d2d-only-roadmap.md`  
> （全部自绘控件改 D2D，**移除 GDI+**；`test_view` 与 `d2d_demo` 同后端。）

CMake 同时构建：

| 目标 | 说明 |
|------|------|
| `auralite_d2d` | 下一代 `auralite::Canvas` / `Image`（D2D + DirectWrite + WIC） |
| `AuraLite.Base` / `AuraLite.UI` | Views 静态库（源列表由 `cmake/LegacySources.cmake` 生成；自绘走 Direct2D） |
| `d2d_demo` | HWND 冒烟：圆角/矩形/文字/位图 |
| `test_base` / `test_view` | 与 `library.sln` 相同冒烟程序 |

产物统一到与 VS 相同目录：`bin|lib/<Platform>/<Config>/`（例如 `bin\x64\Debug\`）。

```powershell
cd family_win_desktop\3rd-party\AuraLite
python scripts\gen_cmake_sources.py   # vcxproj 变更后重跑
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
.\bin\x64\Debug\d2d_demo.exe
.\bin\x64\Debug\test_view.exe
.\bin\x64\Debug\test_base.exe
```

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
| Checkbox / RadioButton / Switch | pass | `FillRect` / `DrawRect` / `DrawStringInt`；圆点/滑块仍为矩形近似 |
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
