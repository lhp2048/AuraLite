# AuraLite 布局说明（`auralite::ui`）

YAML 与 C++ fluent 共用同一套规则。**父控件决定听哪套语言**：流式容器忽略锚点；`Absolute` 才用锚点。写了但父亲不认的属性会静默无效。

对齐按**屏幕轴**命名：`h_align` 永远是左右，`v_align` 永远是上下。不做百分比尺寸、不做 `elevation` / `z-index`。叠层 = 兄弟在 `children` 里的声明顺序（后画、先命中）。

---

## 先选容器

| 要做的事 | 用 |
|----------|----|
| 从上往下排 | `Column` |
| 从左往右排 | `Row` |
| 工具箱 / 宫格 | `Tile` |
| 多页切换 | `Tab` |
| 左右分栏、拖分割条 | `SplitView` |
| 叠层、贴边、浮窗、自由放置 | `Absolute` |
| 滚动 | `ScrollView`（包一层内容） |

浮动控件不要丢进 `Column`/`Row` 再写 `left`/`x`，无效。外包一层 `Absolute`。

---

## 尺寸：Fill / Hug / Fixed

每个控件每轴一个策略（YAML `width` / `height`）：

| 值 | 含义 |
|----|------|
| `fill` | 吃满父布局在该轴给出的空间 |
| `hug` | 按内容固有尺寸 |
| 数字（如 `100`） | 固定像素（DIP） |

省略时看控件默认。常见默认：**Button / TextField 宽 fill、高 fixed**；**Checkbox hug×hug**；**Label 宽 fill、高 hug**。横排工具按钮请显式 `width: hug`。

---

## `h_align` / `v_align`

取值 `start` | `center` | `end`。水平 start=左、end=右；垂直 start=上、end=下。

不是 Label 的文本 `align`。

| 写在谁身上 | `h_align` | `v_align` |
|------------|-----------|-----------|
| **Column** | 孩子们默认左右怎么齐 | 无 `height: fill` 孩子时，整包内容上下靠哪 |
| **Row** | 无 `width: fill` 孩子时，整包内容左右靠哪 | 孩子们默认上下怎么齐 |
| **子控件** | 覆盖 Column / Absolute 水平自由轴 | 覆盖 Row / Absolute 垂直自由轴 |

父亲只读自己那一轴：Column 忽略孩子的 `v_align`，Row 忽略孩子的 `h_align`（和忽略 `left` 同一条规则）。

子项未写时，用容器在该轴上的默认。Fill 拉满的那一轴没有「剩余」，对齐看不见。

```yaml
# 整页正中一颗按钮
Column:
  width: fill
  height: fill
  h_align: center
  v_align: center
  children:
    - Button: { text: "确定", width: hug }

# Column 里默认靠左，这一颗靠右（必须 hug）
- Button: { text: "关闭", width: hug, h_align: end }

# 工具条垂直居中
Row:
  v_align: center
  children:
    - Label: { text: "名称", width: hug }
    - Button: { text: "…", width: hug }

# Absolute：钉住水平，垂直自由 → 靠右垂直居中
- Button: { text: "全局浮层", right: 12, v_align: center, width: hug, height: 72 }
```

C++：`.h_align(Align::Center)` / `.v_align(Align::Center)`。

**写了也没效果：**

- Column 里孩子 `width: fill`（Button 默认就是）→ 先 `width: hug` 再 `h_align`。
- Absolute 已写 `top`/`bottom`/`y` → `v_align` 管不到垂直。
- 有 fill 孩子时，Column 的 `v_align` / Row 的 `h_align` 打包不生效（剩余已被 weight 分完）。

---

## Column / Row

| 属性 | 作用 |
|------|------|
| `weight` | 仅主轴 **`fill`** 的子项分剩余；Fixed/Hug 上的 weight **忽略** |
| `h_align` / `v_align` | 见上表 |
| `spacing` / `padding` | 间隙与内边距 |

```yaml
Column:
  spacing: 8
  children:
    - Label: { text: "上", height: hug }
    - ScrollView: { height: fill, weight: 1 }
    - Button: { text: "确定", height: 36 }
```

两个 `fill` 兄弟按 weight 比分（都省略则均分）。

---

## Absolute

每轴独立。优先级：**双边锚点 > 单边锚点 > `x`/`y` > `h_align`/`v_align` > 原点 0**。

| 写法 | 结果 |
|------|------|
| `left` + `right` | 水平拉满（减两边距），**忽略**该轴 `width` |
| `top` + `bottom` | 垂直拉满，**忽略**该轴 `height` |
| 只写一边锚点 | 钉住那一边，另一边用自有尺寸（`fill` 则拉到对边） |
| `x` / `y` | 无锚点时的原点偏移 |
| 某轴完全自由 | 才看 `h_align` 或 `v_align`，否则 0 |

双边锚点与 `width: 100` 同时写时**锚点赢**，无编译期/加载期提示。`left+right` 配 `height: 48` 可以（不同轴）。

叠层：后声明的孩子后画、先命中。没有 elevation。

---

## Tile / Tab / SplitView

- **Tile：** `columns: N` 固定列数；`columns: 0` 按 `item_size` 和可用宽折列。不吃 `h_align` / 锚点。
- **Tab：** 子节点即各页；`headers` 做页签栏。未选中页 `visible=false`。页内再套 `Absolute` 才有页内浮层。
- **SplitView：** `ratio`（0..1）是左栏占（总宽−分割条）的比例。

---

## 怎么选（速查）

1. 表单、工具栏、列表页 → `Column`/`Row` + `fill`/`hug`/`weight`。
2. 左右对齐用 `h_align`，上下对齐用 `v_align`；整组默认写在容器上，单个不同再写在子项上。
3. 贴边浮层、叠在内容上 → 外包 `Absolute`，用锚点；自由轴才 `h_align`/`v_align`。
4. 左右可拖分栏 → `SplitView.ratio`。
5. 不要：百分比宽、`z-index`、在 Column 里写 `left`。

设计原稿（规则来源，用法以本文为准）：

- [`../../docs/superpowers/specs/2026-08-15-auralite-layout-weight-align-design.md`](../../docs/superpowers/specs/2026-08-15-auralite-layout-weight-align-design.md)
- [`../../docs/superpowers/specs/2026-08-15-auralite-absolute-anchors-design.md`](../../docs/superpowers/specs/2026-08-15-auralite-absolute-anchors-design.md)
