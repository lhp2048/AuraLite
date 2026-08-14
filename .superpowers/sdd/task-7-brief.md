### Task 7: 阶段一收口文档与里程碑

**Files:**
- Modify: `family_win_desktop/3rd-party/AuraLite/README.md`
- Modify: 本文勾选状态；可选更新 `scripts/_plan_extract.txt` 旁注「以修订计划为准」

- [x] **Step 1:** README「阶段一」改为本文完成定义，并链到本文件
- [x] **Step 2:** 记录已知例外：`Native*` / 系统菜单
- [x] **Step 3:** 标注阶段二入口：`ViewFactory` + yaml-cpp（渲染已 D2D，无需再迁画布）

---

## 3. 阶段二～四（调整后摘要，实施时另开 plan）

### 阶段二 — 声明式 UI 核心
- 集成 yaml-cpp；`ViewFactory`；属性注入；C++ 链式 API
- `Column` / `Row` + padding/spacing
- Demo：`login_window.yaml` 与链式版一致  
- **依赖：** 阶段一 D2D Views 已稳定

### 阶段三 — 异步与响应式
- message_framework 协程 Awaiter；UI 线程安全投递
- 轻量依赖追踪 / 自动刷新

### 阶段四 — 工具链
- YAML 热重载；控件树/布局框/帧率调试面板；API 与 Mini YAML 文档

---

## 4. 风险与应对

| 风险 | 影响 | 应对 |
|------|------|------|
| `gfx::Canvas` API 面过大，一次迁不完 | 进度拖延 | Task 1 先覆盖控件实际调用的子集；`rg Draw` 统计后按使用频率实现 |
| ClearType + 透明层 | 文字发糊 | 不透明主 RT；与旧 GDI+ 踩坑同样避免错误 alpha 合成 |
| Textfield 选区/插入符依赖像素测量 | 光标错位 | Task 2 先锁定 DirectWrite 测宽与 `DrawStringInt` 同一 layout |
| Native 控件仍是 GDI 系统绘制 | 观感不一致 | 阶段一 demo 避免主路径依赖；文档标明例外 |
| Family Shell 仍链 `1.x` | 短期两套后端 | 不强制 Shell 跟 master，直至阶段一验收 + 迁移窗口 |

---

## 5. 里程碑检查表（阶段一）

- [x] `CreateCanvas` 仅 D2D
- [x] `WidgetWin` 回屏 D2D
- [x] 验收清单全部控件通过
- [x] 仓库无 GDI+ 链接与绘制调用
- [x] `test_view` 与 `d2d_demo` 文字观感同一档
- [x] README / 本计划勾选更新

---

## Self-Review

1. **Spec coverage:** 「全部控件替换」「不能保留 GDI」「全用 D2D」→ Task 1–6；声明式及以后 → §3。  
2. **Placeholder scan:** 无 TBD；Native 例外已写明。  
3. **Type consistency:** 统一经 `gfx::Canvas`；工厂与 `WidgetWin::contents_` 均指向 D2D 实现。
