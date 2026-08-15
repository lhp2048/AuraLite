# AuraLite 编码风格

本仓库**没有** `.clang-format` / clang-tidy 强制配置；风格以本文 + **相邻文件**为准。

来源：早期 Chromium `base` / `gfx` / MessageLoop / Views 裁剪移植。  
适用范围（默认）：

| 目录 | 是否按本文 |
|------|------------|
| `base/` | 是 |
| `gfx/` | 是 |
| `message_framework/` | 是 |
| `view_framework/` | 是 |
| 其它与上述同栈的 legacy 源 | 是 |

**例外：** `auralite/`（含 `auralite::ui` / `reactive` / `async`）为 master 新栈，见文末「新栈差异」。改哪棵树，跟哪套风格；**禁止**在 legacy 里写成 fluent UI 风格，也**禁止**在 `auralite::ui` 里强行套 `GetXxx` + 大括号独占行。

---

## 1. 语言与构建

- Legacy 目标（`AuraLite.Base` / `AuraLite.UILegacy`）：**C++14**（源码可偏 C++11 子集）。
- 新 UI 目标（`AuraLite::UI` / `auralite_ui`）：**C++20**。
- 平台：Windows；`UNICODE` / `_UNICODE`；注意 `NOMINMAX`。
- 新改动优先可编译、可读；不做无关全库格式化。

---

## 2. 文件与头文件

- **文件名：** `snake_case.h` / `.cpp`（如 `at_exit.h`、`message_loop.cpp`）。
- **头文件保护（两者都要）：**

```cpp
#ifndef __at_exit_h__
#define __at_exit_h__

#pragma once

// …

#endif //__at_exit_h__
```

- Include 顺序习惯：对应标准库 → 本模块/`base`/`gfx` → 其它工程头。用引号路径（`"base/ref_counted.h"`、`"gfx/rect.h"`）。
- 实现与声明分离；模板/薄包装可全在头文件。

---

## 3. 命名空间

| 区域 | 命名空间 |
|------|----------|
| `base/` | `base::` |
| `gfx/` | `gfx::` |
| `view_framework/` | `view::` |
| `message_framework/` | 多数泵/代理在 `base::`；**`MessageLoop` 常在全局命名空间**（与上游 Chromium 同期一致，新增同类类型时跟旁边文件，勿擅自「纠正」进 `base::`） |

命名空间花括号风格：

```cpp
namespace base
{

    class AtExitManager
    {
        // …
    };

} //namespace base
```

---

## 4. 类型与命名

| 类别 | 规则 | 示例 |
|------|------|------|
| 类 / 结构 / 枚举类型 | `PascalCase` | `AtExitManager`、`FontStyle` |
| 枚举值 | `UPPER_SNAKE` 或与旁文件一致 | `NORMAL`、`BOLD`、`TYPE_UI` |
| 函数 / 方法 | **`PascalCase`，动词开头** | `GetHeight`、`PostTask`、`SetNestableTasksAllowed` |
| 局部变量 / 参数 | `snake_case` 或短名 | `delay_ms`、`font_name` |
| 数据成员 | **尾下划线** `name_` | `lock_`、`bounds_`、`platform_font_` |
| 常量 | `k` + Pascal 或全大写（跟旁文件） | `kHighResolutionTimerModeLeaseTimeMs` |
| 宏 | 全大写；拷贝禁用用既有宏 | `DISALLOW_COPY_AND_ASSIGN(Type)` |

### 访问器习惯

- 主风格：`GetFoo()` / `SetFoo(...)`。
- Chromium Views 遗留混用允许保留：小写 `bounds()`、`font()`，或 `set_border(...)`。  
  **新 API 优先 `Get`/`Set`；** 若类里已是 `set_`/`foo()` 混搭，与同类方法保持一致，勿在同一类内再引入第三种。

---

## 5. 类布局

典型顺序：

1. `public:` 类型别名 / 嵌套类型 / 枚举  
2. 构造 / 析构  
3. 公有方法  
4. `protected:`  
5. `private:` 成员与实现细节  
6. 类末尾：`DISALLOW_COPY_AND_ASSIGN(ClassName);`（需要禁止拷贝时）

引用计数类型沿用 `base::RefCounted` / `RefCountedThreadSafe` / `scoped_refptr`，不要随意换成 `std::shared_ptr`（除非该文件已是新写法且旁文件一致）。

---

## 6. 格式（跟现有文件）

- 缩进：空格；宽度与**当前文件**一致（多数 legacy 为 4）。
- 大括号：**类、命名空间、函数**倾向另起一行 `{`（K&R/Allman 混在 Chromium 老代码中，以本文件既有为准）。
- 行宽：不强制 80；过长则在参数/运算符处断行，对齐旁文件。
- 指针/引用：`Type* p`、`Type& r`（`*`/`&` 靠类型名）在老代码中常见，改文件时与该文件一致。

---

## 7. 注释与字符串

- 允许中文注释（本库历史如此）；公共 API 说明「做什么 / 线程约束 / 所有权」。
- 文件编码：源文件按仓库现有习惯；新增优先 **UTF-8**。若编辑器对旧文件乱码，不要整文件转码除非专门任务。
- UI / 路径：宽字符 `std::wstring` / `wchar_t` 与 Win32 API 对齐。

---

## 8. 线程与生命周期（风格相关约定）

- `MessageLoop` / `MessageLoopProxy`：任务投递与销毁规则跟现有头文件注释；UI 相关逻辑默认在 UI 循环线程。
- 观察者：成对 `Add*` / `Remove*`；析构前移除或文档写明所有权。
- 原始指针所有权必须在注释或命名上说清（`parent_`、`scoped_ptr`/`scoped_refptr` 成员）。

---

## 9. 新栈差异（`auralite/`）

`auralite::ui` / `reactive` / `async` **不**强制本节 §3–§6，而遵循旁文件：

| 项 | 新栈常见写法 |
|----|----------------|
| 标准 | C++20（`auralite_ui`） |
| 命名空间 | `auralite::ui` 等嵌套命名空间，常写 `namespace auralite::ui {` |
| 头文件 | 可仅 `#pragma once` |
| 方法 | fluent / `snake_case`：`fill_width()`、`set_visible()` |
| 成员 | 尾下划线仍常用：`visible_` |
| 绑定 / 协程 | 见 README 阶段三：Signal 仅 UI 线程；MSVC 勿用临时 lambda 协程 |

业务 Theme：绘制路径不散落硬编码色/字体（见阶段二 Theme 设计）。

---

## 10. 评审检查清单（legacy）

- [ ] 文件名 `snake_case`；头文件 `__name_h__` + `#pragma once`
- [ ] 命名空间与目录匹配（注意 `MessageLoop` 全局例外）
- [ ] 成员尾 `_`；禁止拷贝处有 `DISALLOW_COPY_AND_ASSIGN`
- [ ] 新方法偏向 `Get`/`Set`；不与同类现有风格打架
- [ ] 未引入 `auralite::ui` 的 fluent API 进 `view::` / `base::`
- [ ] 注释写清线程与所有权

有冲突时：**同目录最近邻文件 > 本文默认条款**。
