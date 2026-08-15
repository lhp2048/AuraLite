#include "auralite/ui/list_view.h"

#include <algorithm>

namespace auralite::ui {

ListView::ListView() {
  set_focusable(true);
}

ListView& ListView::font_size(float size) {
  font_size_ = size;
  return *this;
}

ListView& ListView::on_selection_changed(SelectionHandler handler) {
  on_selection_ = std::move(handler);
  return *this;
}

int ListView::AddItem(const std::wstring& text) {
  items_.push_back(text);
  return static_cast<int>(items_.size()) - 1;
}

void ListView::ClearItems() {
  items_.clear();
  selected_index_ = -1;
}

void ListView::set_selected_index(int index) {
  if (index < -1 || index >= item_count()) {
    return;
  }
  if (selected_index_ == index) {
    return;
  }
  selected_index_ = index;
  if (on_selection_) {
    on_selection_(selected_index_);
  }
}

float ListView::ItemHeight() const {
  return std::max(kMinItemHeight, font_size_ + kItemPaddingY * 2.f);
}

int ListView::IndexAtY(float y) const {
  if (items_.empty()) {
    return -1;
  }
  const float rel = y - bounds_.y;
  if (rel < 0.f) {
    return -1;
  }
  const int idx = static_cast<int>(rel / ItemHeight());
  if (idx < 0 || idx >= item_count()) {
    return -1;
  }
  return idx;
}

SizeF ListView::Measure(float max_w, float /*max_h*/) {
  const float h = ItemHeight() * static_cast<float>(items_.size());
  // Approximate width from longest string; Layout stretches in Column/ScrollView.
  float max_text_w = 40.f;
  for (const auto& t : items_) {
    max_text_w = std::max(
        max_text_w, font_size_ * 0.55f * static_cast<float>(t.size()));
  }
  const float w = std::min(max_w, max_text_w + kItemPaddingX * 2.f);
  return SizeF{w > 0.f ? w : max_w, h};
}

void ListView::Paint(auralite::Canvas& canvas) {
  const float ih = ItemHeight();
  for (int i = 0; i < item_count(); ++i) {
    const RectF row{bounds_.x, bounds_.y + static_cast<float>(i) * ih,
                    bounds_.w, ih};
    const bool selected = (i == selected_index_);
    if (selected) {
      canvas.FillRect(row, selected_bg_);
    }
    const ColorF& color = selected ? selected_text_ : text_color_;
    const RectF text_rect{row.x + kItemPaddingX, row.y,
                          std::max(0.f, row.w - kItemPaddingX * 2.f), row.h};
    canvas.DrawText(items_[static_cast<size_t>(i)], text_rect, color,
                    font_size_, L"Microsoft YaHei UI",
                    auralite::TextHAlign::Left);
  }
  if (focused()) {
    canvas.DrawRect(bounds_, ColorF::FromRgb(40, 110, 200), 1.5f);
  }
}

void ListView::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  const int idx = IndexAtY(e.y);
  if (idx >= 0) {
    set_selected_index(idx);
  }
}

void ListView::OnKey(const KeyEvent& e) {
  if (!e.down || items_.empty()) {
    return;
  }
  if (e.vk == VK_UP) {
    if (selected_index_ < 0) {
      set_selected_index(0);
    } else if (selected_index_ > 0) {
      set_selected_index(selected_index_ - 1);
    }
  } else if (e.vk == VK_DOWN) {
    if (selected_index_ < 0) {
      set_selected_index(0);
    } else if (selected_index_ + 1 < item_count()) {
      set_selected_index(selected_index_ + 1);
    }
  } else if (e.vk == VK_HOME) {
    set_selected_index(0);
  } else if (e.vk == VK_END) {
    set_selected_index(item_count() - 1);
  }
}

}  // namespace auralite::ui
