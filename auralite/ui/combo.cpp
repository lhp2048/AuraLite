#include "auralite/ui/combo.h"

#include "auralite/ui/list_view.h"
#include "auralite/ui/window.h"

#include <algorithm>

namespace auralite::ui {

Combo::Combo() {
  set_focusable(true);
  fill_width();
  fixed_height(36.f);
}

void Combo::BindWindow(Window* window) { window_ = window; }

Combo& Combo::items(std::vector<std::wstring> values) {
  items_ = std::move(values);
  if (selected_ >= static_cast<int>(items_.size())) {
    selected_ = items_.empty() ? -1 : 0;
  }
  return *this;
}

Combo& Combo::add_item(std::wstring text) {
  items_.push_back(std::move(text));
  if (selected_ < 0 && !items_.empty()) {
    selected_ = 0;
  }
  return *this;
}

Combo& Combo::selected(int index) {
  SelectIndex(index, false);
  return *this;
}

Combo& Combo::on_changed(ChangeHandler handler) {
  on_changed_ = std::move(handler);
  return *this;
}

Combo& Combo::font_size(float size) {
  font_size_ = size;
  return *this;
}

void Combo::SelectIndex(int index, bool notify) {
  if (items_.empty()) {
    selected_ = -1;
    return;
  }
  selected_ = std::clamp(index, 0, static_cast<int>(items_.size()) - 1);
  if (notify && on_changed_) {
    on_changed_(selected_);
  }
}

void Combo::ClosePopup() {
  open_ = false;
  if (window_ && window_->popup()) {
    window_->ClearPopup();
  }
}

void Combo::OpenPopup() {
  if (!window_ || items_.empty()) {
    return;
  }
  if (open_) {
    ClosePopup();
    return;
  }

  auto list = std::make_unique<ListView>();
  list->font_size(font_size_);
  for (const auto& t : items_) {
    list->AddItem(t);
  }
  if (selected_ >= 0) {
    list->set_selected_index(selected_);
  }
  list->on_selection_changed([this](int index) {
    SelectIndex(index, true);
    open_ = false;
    if (window_) {
      // Defer destroy: ListView is still on the mouse-down stack.
      window_->RequestClearPopup();
    }
  });

  const SizeF want = list->Measure(bounds_.w, 200.f);
  const float h = std::min(want.h, 180.f);
  list->fixed_height(h);
  list->Layout(RectF{bounds_.x, bounds_.y + bounds_.h + 2.f, bounds_.w, h});

  open_ = true;
  window_->SetPopup(std::move(list), [this]() { open_ = false; }, this);
}

SizeF Combo::Measure(float max_w, float max_h) {
  return ResolveSize(max_w, max_h,
                     preferred_width() > 0.f ? preferred_width() : max_w, 36.f);
}

void Combo::Paint(auralite::Canvas& canvas) {
  canvas.FillRoundedRect(bounds_, 6.f, 6.f, ColorF::FromRgb(255, 255, 255));
  canvas.DrawRect(bounds_, focused() ? ColorF::FromRgb(40, 110, 200)
                                     : ColorF::FromRgb(170, 180, 195),
                  focused() ? 1.5f : 1.f);

  std::wstring label = L"（未选择）";
  if (selected_ >= 0 && selected_ < static_cast<int>(items_.size())) {
    label = items_[static_cast<size_t>(selected_)];
  }
  const RectF text_rect{bounds_.x + 10.f, bounds_.y,
                        std::max(0.f, bounds_.w - 36.f), bounds_.h};
  canvas.DrawText(label, text_rect, ColorF::FromRgb(25, 35, 50), font_size_,
                  L"Microsoft YaHei UI", auralite::TextHAlign::Left);

  // Chevron
  const float cx = bounds_.x + bounds_.w - 18.f;
  const float cy = bounds_.y + bounds_.h * 0.5f;
  canvas.DrawLine(cx - 5.f, cy - 2.f, cx, cy + 3.f, ColorF::FromRgb(80, 90, 110),
                  1.5f);
  canvas.DrawLine(cx, cy + 3.f, cx + 5.f, cy - 2.f, ColorF::FromRgb(80, 90, 110),
                  1.5f);
}

void Combo::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  OpenPopup();
}

void Combo::OnKey(const KeyEvent& e) {
  if (!e.down || items_.empty()) {
    return;
  }
  if (e.vk == VK_ESCAPE && open_) {
    ClosePopup();
    return;
  }
  if (e.vk == VK_RETURN || e.vk == VK_SPACE) {
    OpenPopup();
    return;
  }
  if (e.vk == VK_DOWN) {
    SelectIndex(selected_ < 0 ? 0 : selected_ + 1, true);
  } else if (e.vk == VK_UP) {
    SelectIndex(selected_ < 0 ? 0 : selected_ - 1, true);
  }
}

}  // namespace auralite::ui
