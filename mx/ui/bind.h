#pragma once

#include "mx/reactive/observe.h"
#include "mx/reactive/signal.h"
#include "mx/ui/button.h"
#include "mx/ui/checkbox.h"
#include "mx/ui/image_button.h"
#include "mx/ui/item_list.h"
#include "mx/ui/label.h"
#include "mx/ui/node.h"
#include "mx/ui/progress_bar.h"
#include "mx/ui/slider.h"
#include "mx/ui/theme.h"
#include "mx/ui/virtual_list.h"
#include "mx/ui/window.h"

#include <algorithm>
#include <string>
#include <vector>

namespace mx::ui {

namespace detail {

inline void InvalidateHost(Node& node) {
  if (Window* w = node.host_window()) {
    w->Invalidate();
  }
}

}  // namespace detail

template <typename TextSrc>
mx::reactive::Subscription BindText(Label& label, TextSrc& src) {
  return mx::reactive::Observe([&label, &src] {
    label.text(src.Get());
    detail::InvalidateHost(label);
  });
}

template <typename TextSrc>
mx::reactive::Subscription BindText(Button& button, TextSrc& src) {
  return mx::reactive::Observe([&button, &src] {
    button.text(src.Get());
    detail::InvalidateHost(button);
  });
}

template <typename BoolSrc>
mx::reactive::Subscription BindVisible(Node& node, BoolSrc& src) {
  return mx::reactive::Observe([&node, &src] {
    node.set_visible(static_cast<bool>(src.Get()));
    detail::InvalidateHost(node);
  });
}

template <typename BoolSrc>
mx::reactive::Subscription BindEnabled(Button& button, BoolSrc& src) {
  return mx::reactive::Observe([&button, &src] {
    button.set_enabled(static_cast<bool>(src.Get()));
    detail::InvalidateHost(button);
  });
}

template <typename BoolSrc>
mx::reactive::Subscription BindEnabled(ImageButton& button, BoolSrc& src) {
  return mx::reactive::Observe([&button, &src] {
    button.set_enabled(static_cast<bool>(src.Get()));
    detail::InvalidateHost(button);
  });
}

template <typename BoolSrc>
mx::reactive::Subscription BindChecked(Checkbox& box, BoolSrc& src) {
  return mx::reactive::Observe([&box, &src] {
    box.checked(static_cast<bool>(src.Get()));
    detail::InvalidateHost(box);
  });
}

template <typename FloatSrc>
mx::reactive::Subscription BindValue(ProgressBar& bar, FloatSrc& src) {
  return mx::reactive::Observe([&bar, &src] {
    bar.value(static_cast<float>(src.Get()));
    detail::InvalidateHost(bar);
  });
}

template <typename FloatSrc>
mx::reactive::Subscription BindValue(Slider& slider, FloatSrc& src) {
  return mx::reactive::Observe([&slider, &src] {
    slider.value(static_cast<float>(src.Get()));
    detail::InvalidateHost(slider);
  });
}

template <typename BoolSrc>
mx::reactive::Subscription BindIndeterminate(ProgressBar& bar,
                                                   BoolSrc& src) {
  return mx::reactive::Observe([&bar, &src] {
    bar.indeterminate(static_cast<bool>(src.Get()));
    detail::InvalidateHost(bar);
  });
}

// Whole-vector replace into VirtualList Text rows.
inline mx::reactive::Subscription BindItems(
    VirtualList& list,
    mx::reactive::Signal<std::vector<std::wstring>>& items) {
  list.item_kind([](int) { return VirtualListItemKind::Text; });
  list.item_count([&items] {
    return static_cast<int>(items.Peek().size());
  });
  list.item_text([&items](int i) -> std::wstring {
    const auto& v = items.Peek();
    if (i < 0 || i >= static_cast<int>(v.size())) {
      return {};
    }
    return v[static_cast<size_t>(i)];
  });
  return mx::reactive::Observe([&list, &items] {
    (void)items.Get();
    list.InvalidateData();
    detail::InvalidateHost(list);
  });
}

// Whole-vector replace into ItemList (default text paint from |items|).
inline mx::reactive::Subscription BindItems(
    ItemList& list,
    mx::reactive::Signal<std::vector<std::wstring>>& items) {
  list.on_paint_item([&items](mx::Canvas& canvas, const RectF& row,
                              const ItemListRowState& state) {
    const ThemeTokens& th = Theme::Active();
    if (state.selected) {
      canvas.FillRect(row, th.accent);
    } else if (state.hovered) {
      canvas.FillRect(row, th.accent_soft);
    }
    const auto& v = items.Peek();
    std::wstring text;
    if (state.index >= 0 && state.index < static_cast<int>(v.size())) {
      text = v[static_cast<size_t>(state.index)];
    }
    const ColorF tc = state.selected ? th.text_on_accent : th.text;
    canvas.DrawText(text,
                    RectF{row.x + 8.f, row.y, std::max(0.f, row.w - 16.f), row.h},
                    tc, th.font_size, th.font_ui.c_str(),
                    mx::TextHAlign::Left);
  });
  return mx::reactive::Observe([&list, &items] {
    const auto& v = items.Get();
    list.set_item_count(static_cast<int>(v.size()));
    list.InvalidateBinds();
    detail::InvalidateHost(list);
  });
}

}  // namespace mx::ui
