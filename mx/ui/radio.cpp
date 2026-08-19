#include "mx/ui/radio.h"

#include "mx/ui/theme.h"

#include <algorithm>

namespace mx::ui {

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
  NotifyAccToggleChanged();
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
      radio->NotifyAccToggleChanged();
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

SizeF Radio::Measure(float max_w, float max_h) {
  const ThemeTokens& th = Theme::Active();
  const float fs = ResolveFontSize(font_size_);
  const float text_w =
      text_.empty() ? 0.f
                    : mx::MeasureUiTextWidth(text_, fs, th.font_ui.c_str());
  const float hug_w = kDotSize + (text_.empty() ? 0.f : kLabelGap + text_w);
  const float hug_h = std::max(kDotSize, fs + 6.f);
  return ResolveSize(max_w, max_h, hug_w, hug_h);
}

void Radio::Paint(mx::Canvas& canvas) {
  if (!visible()) {
    return;
  }
  const ThemeTokens& th = Theme::Active();
  const RectF outer = DotRect();
  canvas.DrawEllipse(outer, th.border, 1.5f);

  if (checked_) {
    const float inset = 4.f;
    const RectF inner{outer.x + inset, outer.y + inset, outer.w - inset * 2.f,
                      outer.h - inset * 2.f};
    canvas.FillEllipse(inner, th.glyph);
  }

  if (!text_.empty()) {
    const float text_x = outer.x + kDotSize + kLabelGap;
    const RectF text_rect{text_x, bounds_.y,
                          std::max(0.f, bounds_.x + bounds_.w - text_x),
                          bounds_.h};
    canvas.DrawText(text_, text_rect, th.text, ResolveFontSize(font_size_),
                    th.font_ui.c_str(), mx::TextHAlign::Left);
  }

  if (focused()) {
    canvas.DrawDashedRect(bounds_, th.border_focus, 1.f);
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

AccRole Radio::acc_role() const {
  return AccRole::RadioButton;
}

std::wstring Radio::AccDefaultName() const {
  return text_;
}

AccState Radio::acc_state() const {
  AccState s = Node::acc_state();
  s.checked = checked_;
  return s;
}

bool Radio::AccInvoke() {
  Select();
  return true;
}

bool Radio::AccToggle() {
  Select();
  return true;
}

}  // namespace mx::ui
