#include "auralite/ui/radio.h"

#include <algorithm>

namespace auralite::ui {

Radio::Radio() {
  set_focusable(true);
}

Radio& Radio::text(const std::wstring& t) {
  text_ = t;
  return *this;
}

Radio& Radio::font_size(float size) {
  font_size_ = size;
  return *this;
}

Radio& Radio::group_id(int id) {
  group_id_ = id;
  return *this;
}

Radio& Radio::checked(bool v) {
  SetCheckedInternal(v, true);
  return *this;
}

Radio& Radio::on_changed(ChangedHandler handler) {
  on_changed_ = std::move(handler);
  return *this;
}

void Radio::SetCheckedInternal(bool v, bool notify) {
  if (checked_ == v) {
    return;
  }
  checked_ = v;
  if (checked_) {
    UncheckGroupPeers();
  }
  if (notify && on_changed_) {
    on_changed_(checked_);
  }
}

void Radio::UncheckRadiosInTree(Node* node, Radio* except, int group) {
  if (!node) {
    return;
  }
  if (auto* radio = dynamic_cast<Radio*>(node)) {
    if (radio != except && radio->group_id_ == group && radio->checked_) {
      radio->checked_ = false;
    }
  }
  for (const auto& child : node->children()) {
    UncheckRadiosInTree(child.get(), except, group);
  }
}

void Radio::UncheckGroupPeers() {
  Node* root = this;
  while (root->parent()) {
    root = root->parent();
  }
  UncheckRadiosInTree(root, this, group_id_);
}

RectF Radio::DotRect() const {
  const float y = bounds_.y + (bounds_.h - kDotSize) * 0.5f;
  return RectF{bounds_.x, y, kDotSize, kDotSize};
}

SizeF Radio::Measure(float /*max_w*/, float /*max_h*/) {
  float text_w = 0.f;
  if (!text_.empty()) {
    text_w = font_size_ * 0.55f * static_cast<float>(text_.size());
  }
  const float w = kDotSize + (text_.empty() ? 0.f : kLabelGap + text_w);
  const float h = std::max(kDotSize, font_size_ + 6.f);
  return SizeF{w, h};
}

void Radio::Paint(auralite::Canvas& canvas) {
  const RectF outer = DotRect();
  canvas.DrawEllipse(outer, ColorF::FromRgb(60, 60, 60), 1.5f);

  if (checked_) {
    const float inset = 4.f;
    const RectF inner{outer.x + inset, outer.y + inset, outer.w - inset * 2.f,
                      outer.h - inset * 2.f};
    canvas.FillEllipse(inner, ColorF::FromRgb(40, 110, 200));
  }

  if (!text_.empty()) {
    const float text_x = outer.x + kDotSize + kLabelGap;
    const RectF text_rect{text_x, bounds_.y,
                          std::max(0.f, bounds_.x + bounds_.w - text_x),
                          bounds_.h};
    canvas.DrawText(text_, text_rect, ColorF::FromRgb(30, 40, 55), font_size_,
                    L"Microsoft YaHei UI", auralite::TextHAlign::Left);
  }

  if (focused()) {
    canvas.DrawRect(bounds_, ColorF::FromRgb(40, 110, 200), 1.5f);
  }
}

void Radio::Select() {
  SetCheckedInternal(true, true);
}

void Radio::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  pressed_ = true;
}

void Radio::OnMouseUp(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  const bool was_pressed = pressed_;
  pressed_ = false;
  if (was_pressed && ContainsPoint(bounds_, e.x, e.y)) {
    Select();
  }
}

void Radio::OnKey(const KeyEvent& e) {
  if (!e.down) {
    return;
  }
  if (e.vk == VK_SPACE || e.vk == VK_RETURN) {
    Select();
  }
}

}  // namespace auralite::ui
