# AuraLite

Windows UI 工具库（源自早期 Chromium Views），作为第三方依赖放在 `family_win_desktop/3rd-party/AuraLite`。

上游仓库：https://github.com/lhp2048/AuraLite

## 工程结构

源码目录仍按模块划分；**对外只编两个静态库**：

| 工程 | 聚合内容 | 产物 |
|------|----------|------|
| **AuraLite.Base** | `base` + `rfc_algorithm` + `message_framework` | `AuraLite.Base.lib` |
| **AuraLite.UI** | `gfx` + `animation` + `view_framework` | `AuraLite.UI.lib` |
| `test_base` | Base 冒烟（控制台） | `test_base.exe` |
| `test_view` | UI 示例（窗口） | `test_view.exe` |

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
2. 链接：`AuraLite.UI.lib` + `AuraLite.Base.lib`（及系统库：`gdiplus`、`ole32`、`oleacc`、`dwmapi`、`uxtheme` 等，参见 `test_view`）
3. 预处理器建议定义：`AURALITE_STATIC`、`NOMINMAX`、`_WIN32_WINNT=0x0601`

## 依赖说明

- **默认不依赖 ATL / MFC**。无障碍相关使用 MSAA stub（`view_accessibility_msaa.cpp`）。
- 若本机安装了 VC ATL/MFC，可定义 `AURALITE_HAS_ATL` 并恢复完整 ATL 无障碍实现（可选）。
- UI 绘制基于 **GDI+**（见 `gfx/`）。
