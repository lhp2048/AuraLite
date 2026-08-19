#include "mx/ui/list_view.h"

#include "mx/ui/theme.h"

#include <algorithm>

namespace mx::ui {

ListView::ListView() {
  set_focusable(true);
  fill_width();
  hug_height();
}

AccRole ListView::acc_role() const {
  if (acc_role_override_) {
    return *acc_role_override_;
  }
  return AccRole::List;
}

ListView& ListView::font_size(float size) {
  font_size_ = size;
  return *this;
}

ListView& ListView::text_color(const ColorF& c) {
  text_color_ = c;
  return *this;
}

ListView& ListView::selected_bg(const ColorF& c) {
  selected_bg_ = c;
  return *this;
}

ListView& ListView::selected_text(const ColorF& c) {
  selected_text_ = c;
  return *this;
}

ListView& ListView::hover_bg(const ColorF& c) {
  hover_bg_ = c;
  return *this;
}

ListView& ListView::on_selection_changed(SelectionHandler handler) {
  on_selection_ = std::move(handler);
  return *this;
}

ListView& ListView::on_check_changed(CheckHandler handler) {
  on_check_ = std::move(handler);
  return *this;
}

ListView& ListView::checkable(bool enable) {
  checkable_ = enable;
  EnsureCheckedSize();
  return *this;
}

void ListView::EnsureCheckedSize() {
  if (checked_.size() < items_.size()) {
    checked_.resize(items_.size(), false);
  }
}

int ListView::AddItem(const std::wstring& text) {
  items_.push_back(text);
  checked_.push_back(false);
  return static_cast<int>(items_.size()) - 1;
}

void ListView::ClearItems() {
  items_.clear();
  checked_.clear();
  selected_index_ = -1;
  hover_index_ = -1;
}

void ListView::set_hover_index(int index) {
  if (hover_index_ == index) {
    return;
  }
  hover_index_ = index;
  Invalidate();
}

void ListView::CommitSelection() {
  if (on_selection_ && selected_index_ >= 0) {
    on_selection_(selected_index_);
  }
}

void ListView::set_selected_index(int index, bool notify) {
  if (index < -1 || index >= item_count()) {
    return;
  }
  if (selected_index_ == index) {
    if (notify && !checkable_) {
      CommitSelection();
    }
    return;
  }
  selected_index_ = index;
  set_hover_index(index);
  if (notify && !checkable_) {
    CommitSelection();
  }
}

void ListView::set_checked(int index, bool checked) {
  if (index < 0 || index >= item_count()) {
    return;
  }
  EnsureCheckedSize();
  if (checked_[static_cast<size_t>(index)] == checked) {
    return;
  }
  checked_[static_cast<size_t>(index)] = checked;
  if (on_check_) {
    on_check_(index, checked);
  }
}

bool ListView::is_checked(int index) const {
  if (index < 0 || index >= static_cast<int>(checked_.size())) {
    return false;
  }
  return checked_[static_cast<size_t>(index)];
}

void ListView::set_checked_indices(const std::vector<int>& indices) {
  EnsureCheckedSize();
  std::fill(checked_.begin(), checked_.end(), false);
  for (int i : indices) {
    if (i >= 0 && i < item_count()) {
      checked_[static_cast<size_t>(i)] = true;
    }
  }
}

std::vector<int> ListView::checked_indices() const {
  std::vector<int> out;
  for (int i = 0; i < static_cast<int>(checked_.size()); ++i) {
    if (checked_[static_cast<size_t>(i)]) {
      out.push_back(i);
    }
  }
  return out;
}

void ListView::ToggleChecked(int index) {
  if (index < 0 || index >= item_count()) {
    return;
  }
  set_checked(index, !is_checked(index));
}

float ListView::ItemHeight() const {
  return std::max(kMinItemHeight,
                  ResolveFontSize(font_size_) + kItemPaddingY * 2.f);
}

float ListView::CheckBoxSize() const {
  return std::min(16.f, ItemHeight() - 6.f);
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

SizeF ListView::Measure(float max_w, float max_h) {
  const ThemeTokens& th = Theme::Active();
  const float fs = ResolveFontSize(font_size_);
  const float hug_h = ItemHeight() * static_cast<float>(items_.size());
  float max_text_w = 40.f;
  for (const auto& t : items_) {
    max_text_w = std::max(
        max_text_w, mx::MeasureUiTextWidth(t, fs, th.font_ui.c_str()));
  }
  float hug_w = max_text_w + kItemPaddingX * 2.f;
  if (checkable_) {
    hug_w += CheckBoxSize() + 8.f;
  }
  return ResolveSize(max_w, max_h, hug_w, hug_h);
}

void ListView::Paint(mx::Canvas& canvas) {
  if (!visible()) {
    return;
  }
  const ThemeTokens& th = Theme::Active();
  const float fs = ResolveFontSize(font_size_);
  const ColorF text_c = text_color_.value_or(th.text);
  const ColorF sel_bg = selected_bg_.value_or(th.accent);
  const ColorF sel_text = selected_text_.value_or(th.text_on_accent);
  const ColorF hov_bg = hover_bg_.value_or(th.accent_soft);

  canvas.FillRect(bounds_, th.surface);
  canvas.DrawRect(bounds_, th.border, 1.f);
  const float ih = ItemHeight();
  const float box = CheckBoxSize();
  for (int i = 0; i < item_count(); ++i) {
    const RectF row{bounds_.x, bounds_.y + static_cast<float>(i) * ih,
                    bounds_.w, ih};
    const bool selected = (i == selected_index_);
    const bool hovered = (i == hover_index_);
    const bool checked = is_checked(i);
    if (checkable_) {
      if (hovered) {
        canvas.FillRect(row, hov_bg);
      }
      if (selected) {
        canvas.DrawRect(row, th.border_focus, 1.f);
      }
    } else if (selected) {
      canvas.FillRect(row, sel_bg);
    } else if (hovered) {
      canvas.FillRect(row, hov_bg);
    }

    float text_x = row.x + kItemPaddingX;
    if (checkable_) {
      const RectF box_r{row.x + kItemPaddingX, row.y + (ih - box) * 0.5f, box,
                        box};
      canvas.FillRect(box_r, th.surface);
      canvas.DrawRect(box_r, th.border, 1.f);
      if (checked) {
        canvas.DrawLine(box_r.x + 3.f, box_r.y + box * 0.55f,
                        box_r.x + box * 0.4f, box_r.y + box - 3.f, th.glyph,
                        1.8f);
        canvas.DrawLine(box_r.x + box * 0.4f, box_r.y + box - 3.f,
                        box_r.x + box - 3.f, box_r.y + 3.f, th.glyph, 1.8f);
      }
      text_x = box_r.x + box + 8.f;
    }

    const ColorF& color = (!checkable_ && selected) ? sel_text : text_c;
    const RectF text_rect{text_x, row.y,
                          std::max(0.f, row.x + row.w - text_x - kItemPaddingX),
                          row.h};
    canvas.DrawText(items_[static_cast<size_t>(i)], text_rect, color, fs,
                    th.font_ui.c_str(), mx::TextHAlign::Left);
  }
  if (focused()) {
    canvas.DrawRect(bounds_, th.border_focus, 1.5f);
  }
}

void ListView::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  const int idx = IndexAtY(e.y);
  if (idx < 0) {
    return;
  }
  set_hover_index(idx);
  if (checkable_) {
    set_selected_index(idx, false);
    ToggleChecked(idx);
    return;
  }
  set_selected_index(idx, true);
}

void ListView::OnMouseMove(const MouseEvent& e) {
  set_hover_index(IndexAtY(e.y));
}

void ListView::OnMouseLeave(const MouseEvent&) {
  set_hover_index(-1);
}

void ListView::OnKey(const KeyEvent& e) {
  if (!e.down || items_.empty()) {
    return;
  }
  if (e.vk == VK_RETURN || e.vk == VK_SPACE) {
    if (checkable_) {
      if (selected_index_ >= 0) {
        ToggleChecked(selected_index_);
      }
    } else {
      CommitSelection();
    }
    return;
  }
  if (e.vk == VK_UP) {
    if (selected_index_ < 0) {
      set_selected_index(0, false);
    } else if (selected_index_ > 0) {
      set_selected_index(selected_index_ - 1, false);
    }
  } else if (e.vk == VK_DOWN) {
    if (selected_index_ < 0) {
      set_selected_index(0, false);
    } else if (selected_index_ + 1 < item_count()) {
      set_selected_index(selected_index_ + 1, false);
    }
  } else if (e.vk == VK_HOME) {
    set_selected_index(0, false);
  } else if (e.vk == VK_END) {
    set_selected_index(item_count() - 1, false);
  }
}

}  // namespace mx::ui
