# AuraLite 界面库评估

**版本：** 3.x（`master`）  
**评估日期：** 2026-08-19  
**视角：** 作为 **独立 Windows 桌面界面库**，能否支撑「第二个产品」只靠本库交付 UI（非 Family 壳专用补丁集）。

定位对齐 README：**轻量、现代、快捷**。分数不是和 Qt / WinUI「控件数量」比，而是和**该定位下的完成度**比。

---

## 总评


| 指标          | 分数           | 说明                                       |
| ----------- | ------------ | ---------------------------------------- |
| **综合（界面库）** | **8.4 / 10** | 轻量 Win32 自绘库第一梯队；非全功能超市                  |
| 定位契合度       | 9.0          | 边界表清晰，不做项 intentional                    |
| 第二产品可交付     | 8.0          | 表单 + 列表/树/表格 + 菜单 + 主题够用；Excel 级表格/富文本要绕 |


**上轮参考（约 2026-08-18，无 DataGrid / ColorPicker）：** 综合约 **8.0～8.2**。  
**本轮上调（+0.2～0.4）：** DataGrid、ColorPicker 落地；弹出层/表格交互与表头 resize 光标等打磨。

---

## 分维度


| 维度         | 本轮      | 上轮  | Δ    | 要点                                                 |
| ---------- | ------- | --- | ---- | -------------------------------------------------- |
| 控件完备       | **8.7** | 8.2 | +0.5 | DataGrid v1、ColorPicker；DatePicker/MenuBar 等已齐     |
| 布局系统       | **8.4** | 8.4 | —    | Fill/Hug/Fixed + 锚点；无百分比/Grid                      |
| 主题样式       | **8.0** | 7.5 | +0.5 | 每窗主题 + YAML 注册 + `theme_picker`                    |
| 渲染后端       | **8.3** | 8.3 | —    | D2D + DIP + 无边框/分层 Popup 分路径                       |
| 输入焦点       | **8.1** | 7.9 | +0.2 | SetPopup 焦点修复；DataGrid 点外提交；列宽 ↔ 光标                |
| 声明式 API    | **8.5** | 8.4 | +0.1 | YAML + DSL + Factory；`ui_gallery --check`          |
| 架构维护       | **8.3** | 8.2 | +0.1 | Node 树 + 静态 lib；Legacy 冻结；Win7 独立开关                |
| 无障碍 UIA    | **7.2** | 6.8 | +0.4 | 控件级 pattern；**无**列表/树/DataGrid 项级                  |
| HiDPI / 打磨 | **8.3** | 8.2 | +0.1 | 表格列线、popup 贴合、resize 命中区                           |
| 文档测试       | **8.5** | 8.5 | —    | README + demo + `widget_test` / layout / acc / uia |


---



## 本轮新增控件



### ColorPicker ✅


| 项    | 状态                                                       |
| ---- | -------------------------------------------------------- |
| 源码   | `auralite/ui/color_picker.{h,cpp}`                       |
| 接入   | `factory` · `dsl` · `ui.h` · CMake                       |
| 能力   | `mode: simple|full`；`alpha`；YAML `#RRGGBB` / `#RRGGBBAA` |
| 要求   | `BindWindow`（与 DatePicker 相同 popup 模型）                   |
| Demo | `gallery.yaml` · `examples/theme_picker`                 |
| 测试   | `widget_test`                                            |


**库级 vs 产品级：** 设置页 / 主题调色足够；非 Photoshop / VS 级专业取色。

### DataGrid ✅


| 项    | 状态                                                           |
| ---- | ------------------------------------------------------------ |
| 源码   | `auralite/ui/data_grid.{h,cpp}`（继承 `VirtualList`）            |
| 接入   | `factory` · `dsl` · `ui.h` · CMake                           |
| 能力   | 表头、列宽拖拽、冻结列、虚拟滚动、内置 `cells_`、文本单元格编辑、**默认排序** |
| 编辑   | 双击 / F2 / Enter；`BindWindow`；点外部提交；Esc 取消                    |
| 列    | YAML `editable: false` 只读；`align` / `width` / `frozen_count` |
| Demo | `gallery.yaml` · fluent `MakeDemoDataGrid`                   |
| 测试   | `widget_test`                                                |


**库级 vs 产品级：**


| 有                           | 无                     |
| --------------------------- | --------------------- |
| 多列展示 + 内存表 + 文本编辑           | 内联 Combo/Check 列      |
| 表头排序 + 自动重排 `cells_`        | Tab 切格、复制粘贴           |
| `sort_compare` / `auto_sort(false)` | 运行时 UI 钉列             |
| 冻结列（`frozen_count`）         |                       |
| 列竖线 + resize 光标             |                       |


---



## 与常见库对照（预期管理）


| 能力          | AuraLite 3.x | Qt Widgets | WinUI 3 |
| ----------- | ------------ | ---------- | ------- |
| 跨平台         | ✗            | ✓          | ✗       |
| 依赖体积        | 小（静态 lib）    | 大          | 中       |
| YAML 声明式    | ✓            | ✗          | ✗       |
| DataGrid    | v1 轻量        | 完整         | 完整      |
| ColorPicker | 简易 + 完整色板    | ✓          | ✓       |
| 可视化设计器      | ✗            | ✓          | ✓       |
| 项级 UIA      | ✗            | 部分         | ✓       |


---



## 仍低于「全功能界面库」的边界

README 边界表 intentional，**不是待办 backlog**：

- 跨平台、触摸手势、命令冒泡
- 样式表 / QSS、控件级 `set_theme`
- 列表/树/DataGrid **项级** UIA、高对比、RTL
- Accordion、富文本、百分比布局、`z-index`
- YAML 双向绑定、`ObservableList` diff

---



## 建议（按性价比）

**可冻结为 1.0 能力面：** 上表「不做」项；Excel 级 DataGrid；专业 ColorPicker。

**值得再收一小口：** （暂无）

> 列竖线（`PaintColumnDividers`）已区分相邻列；单元格 padding 暂无 API，**暂不计划**单独加。  
> Tab 切格（键盘在单元格间跳转）**暂不做**；当前 F2 / Enter + 鼠标编辑够用。  
> DataGrid 表头排序默认重排 `cells_`；`sort_compare` 自定义比较，`auto_sort: false` 时完全手动。

**验证命令：**

```powershell
cmake --build build --config Debug --target ui_gallery widget_test auralite_ui
.\bin\x64\Debug\widget_test.exe
.\bin\x64\Debug\ui_gallery.exe
.\bin\x64\Debug\ui_gallery.exe --check
```

---



## 变更记录


| 日期         | 综合      | 说明                                                  |
| ---------- | ------- | --------------------------------------------------- |
| 2026-08-18 | ~8.0    | DatePicker / SpinBox / MenuBar / 每窗主题 / `ui.h`      |
| 2026-08-19 | **8.4** | **ColorPicker**、**DataGrid v1**；popup 焦点/编辑/光标/列线修复 |


---



## 一句话

> AuraLite 3.x：**8.4 分**的 Windows 轻量界面库——标准件齐、双轨声明式成熟，**DataGrid / ColorPicker 已纳入正式控件面**；短板仍在 enterprise 表格、富文本与项级无障碍，以及非 Win32 生态。

