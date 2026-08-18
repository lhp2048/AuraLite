#include "auralite/ui/menu_item.h"

#include "auralite/ui/theme.h"

#include <algorithm>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace auralite::ui {
namespace {

void DrawGlyph(auralite::Canvas& canvas, const RectF& r, const std::wstring& name,
               const ColorF& color) {
  if (name == L"copy") {
    canvas.DrawRect(RectF{r.x + 4.f, r.y, r.w - 4.f, r.h - 4.f}, color, 1.2f);
    canvas.DrawRect(RectF{r.x, r.y + 4.f, r.w - 4.f, r.h - 4.f}, color, 1.2f);
    return;
  }
  if (name == L"paste") {
    canvas.DrawRect(RectF{r.x + 2.f, r.y + 4.f, r.w - 4.f, r.h - 4.f}, color,
                    1.2f);
    canvas.DrawRect(RectF{r.x + 5.f, r.y, r.w - 10.f, 5.f}, color, 1.2f);
    return;
  }
  if (name == L"folder") {
    canvas.DrawRect(RectF{r.x, r.y + 4.f, r.w, r.h - 4.f}, color, 1.2f);
    canvas.DrawLine(r.x, r.y + 4.f, r.x + 5.f, r.y + 4.f, color, 1.2f);
    canvas.DrawLine(r.x + 5.f, r.y + 4.f, r.x + 8.f, r.y + 1.f, color, 1.2f);
    canvas.DrawLine(r.x + 8.f, r.y + 1.f, r.x + 12.f, r.y + 1.f, color, 1.2f);
    return;
  }
  if (name == L"list") {
    for (int i = 0; i < 3; ++i) {
      const float y = r.y + 3.f + static_cast<float>(i) * 5.f;
      canvas.DrawLine(r.x, y, r.x + r.w, y, color, 1.3f);
    }
    return;
  }
  if (name == L"view") {
    canvas.DrawEllipse(r, color, 1.2f);
    const RectF pupil{r.x + 5.f, r.y + 5.f, 6.f, 6.f};
    canvas.FillEllipse(pupil, color);
    return;
  }
  if (name == L"grid") {
    canvas.DrawRect(RectF{r.x, r.y, 6.f, 6.f}, color, 1.2f);
    canvas.DrawRect(RectF{r.x + 10.f, r.y, 6.f, 6.f}, color, 1.2f);
    canvas.DrawRect(RectF{r.x, r.y + 10.f, 6.f, 6.f}, color, 1.2f);
    canvas.DrawRect(RectF{r.x + 10.f, r.y + 10.f, 6.f, 6.f}, color, 1.2f);
    return;
  }
  canvas.DrawRect(r, color, 1.2f);
}

}  // namespace

MenuItem::MenuItem() {
  set_focusable(true);
  fill_width();
  hug_height();
}

MenuItem& MenuItem::text(const std::wstring& t) {
  text_ = t;
  return *this;
}

MenuItem& MenuItem::icon(std::wstring name) {
  icon_ = std::move(name);
  Invalidate();
  return *this;
}

MenuItem& MenuItem::separator(bool v) {
  separator_ = v;
  set_focusable(!v);
  Invalidate();
  return *this;
}

MenuItem& MenuItem::checkable(bool v) {
  checkable_ = v;
  Invalidate();
  return *this;
}

MenuItem& MenuItem::checked(bool v) {
  SetCheckedInternal(v, false);
  return *this;
}

MenuItem& MenuItem::radio_group(int id) {
  radio_group_ = id;
  Invalidate();
  return *this;
}

MenuItem& MenuItem::font_size(float size) {
  font_size_ = size;
  return *this;
}

MenuItem& MenuItem::text_color(const ColorF& c) {
  text_color_ = c;
  Invalidate();
  return *this;
}

MenuItem& MenuItem::bg_hover(const ColorF& c) {
  hover_bg_ = c;
  Invalidate();
  return *this;
}

MenuItem& MenuItem::on_click(ClickHandler handler) {
  on_click_ = std::move(handler);
  return *this;
}

MenuItem& MenuItem::on_changed(ChangedHandler handler) {
  on_changed_ = std::move(handler);
  return *this;
}

RectF MenuItem::GutterRect() const {
  const float y = bounds_.y + (bounds_.h - kIcon) * 0.5f;
  return RectF{bounds_.x + kPadX, y, kIcon, kIcon};
}

void MenuItem::UncheckMenuItemsInTree(Node* node, MenuItem* except, int group) {
  if (!node) {
    return;
  }
  if (auto* item = dynamic_cast<MenuItem*>(node)) {
    if (item != except && item->radio_group_ == group && item->checked_) {
      item->checked_ = false;
      item->NotifyAccToggleChanged();
      item->Invalidate();
    }
  }
  for (const auto& child : node->children()) {
    UncheckMenuItemsInTree(child.get(), except, group);
  }
}

void MenuItem::UncheckRadioPeers() {
  Node* root = this;
  while (root->parent()) {
    root = root->parent();
  }
  UncheckMenuItemsInTree(root, this, radio_group_);
}

void MenuItem::SetCheckedInternal(bool v, bool notify) {
  if (checked_ == v) {
    return;
  }
  checked_ = v;
  if (checked_ && radio_group_ > 0) {
    UncheckRadioPeers();
  }
  NotifyAccToggleChanged();
  Invalidate();
  if (notify && on_changed_) {
    on_changed_(checked_);
  }
}

void MenuItem::Activate() {
  if (separator_) {
    return;
  }
  if (radio_group_ > 0) {
    SetCheckedInternal(true, true);
  } else if (checkable_) {
    SetCheckedInternal(!checked_, true);
  }
  if (on_click_) {
    on_click_();
  }
}

SizeF MenuItem::Measure(float max_w, float max_h) {
  if (separator_) {
    return ResolveSize(max_w, max_h, 72.f, kSepH);
  }
  const ThemeTokens& th = Theme::Active();
  const float fs = ResolveFontSize(font_size_);
  const float text_w =
      text_.empty() ? 0.f
                    : auralite::MeasureUiTextWidth(text_, fs, th.font_ui.c_str());
  float hug_w = kPadX + kGutter + 8.f + text_w + kPadX;
  if (!icon_.empty() && (checkable_ || radio_group_ > 0)) {
    hug_w += kIcon + 6.f;
  }
  return ResolveSize(max_w, max_h, hug_w, kItemH);
}

void MenuItem::Paint(auralite::Canvas& canvas) {
  if (!visible()) {
    return;
  }
  const ThemeTokens& th = Theme::Active();
  if (separator_) {
    const float y = bounds_.y + bounds_.h * 0.5f;
    canvas.FillRect(RectF{bounds_.x + kPadX, y, std::max(0.f, bounds_.w - kPadX * 2.f),
                          1.f},
                    th.divider);
    return;
  }

  const ColorF fg = text_color_.value_or(th.text);
  const ColorF ic = text_color_.value_or(th.glyph);
  const ColorF line = text_color_.value_or(th.border);
  if (hovered_ || pressed_) {
    canvas.FillRoundedRect(bounds_, 4.f, 4.f,
                           hover_bg_.value_or(th.accent_soft));
  }

  const RectF gutter = GutterRect();
  if (checkable_ && checked_) {
    const float x0 = gutter.x + 2.f;
    const float y0 = gutter.y + 8.f;
    const float x1 = gutter.x + 6.f;
    const float y1 = gutter.y + 12.f;
    const float x2 = gutter.x + 14.f;
    const float y2 = gutter.y + 3.f;
    canvas.DrawLine(x0, y0, x1, y1, ic, 1.8f);
    canvas.DrawLine(x1, y1, x2, y2, ic, 1.8f);
  } else if (radio_group_ > 0) {
    canvas.DrawEllipse(gutter, line, 1.2f);
    if (checked_) {
      const RectF inner{gutter.x + 4.f, gutter.y + 4.f, 8.f, 8.f};
      canvas.FillEllipse(inner, ic);
    }
  } else if (!icon_.empty()) {
    DrawGlyph(canvas, gutter, icon_, ic);
  }

  float text_x = bounds_.x + kPadX + kGutter + 6.f;
  if (!icon_.empty() && (checkable_ || radio_group_ > 0)) {
    const RectF icon_r{text_x, gutter.y, kIcon, kIcon};
    DrawGlyph(canvas, icon_r, icon_, ic);
    text_x = icon_r.x + kIcon + 6.f;
  }

  if (!text_.empty()) {
    const RectF text_r{text_x, bounds_.y,
                       std::max(0.f, bounds_.x + bounds_.w - kPadX - text_x),
                       bounds_.h};
    canvas.DrawText(text_, text_r, fg, ResolveFontSize(font_size_),
                    th.font_ui.c_str(), auralite::TextHAlign::Left);
  }

  if (focused()) {
    canvas.DrawDashedRect(bounds_, th.border_focus, 1.f);
  }
}

void MenuItem::OnMouseEnter(const MouseEvent&) {
  if (separator_) {
    return;
  }
  hovered_ = true;
  Invalidate();
}

void MenuItem::OnMouseLeave(const MouseEvent&) {
  hovered_ = false;
  pressed_ = false;
  Invalidate();
}

void MenuItem::OnMouseDown(const MouseEvent& e) {
  if (separator_ || e.button != MouseButton::Left) {
    return;
  }
  pressed_ = true;
  Invalidate();
}

void MenuItem::OnMouseUp(const MouseEvent& e) {
  if (separator_ || e.button != MouseButton::Left) {
    return;
  }
  const bool was = pressed_;
  pressed_ = false;
  Invalidate();
  if (was && ContainsPoint(bounds_, e.x, e.y)) {
    Activate();
  }
}

void MenuItem::OnKey(const KeyEvent& e) {
  if (separator_ || !e.down) {
    return;
  }
  if (e.vk == VK_SPACE || e.vk == VK_RETURN) {
    Activate();
  }
}

AccRole MenuItem::acc_role() const {
  if (acc_role_override_) {
    return *acc_role_override_;
  }
  if (separator_) {
    return AccRole::Ignore;
  }
  if (radio_group_ > 0) {
    return AccRole::RadioButton;
  }
  if (checkable_) {
    return AccRole::CheckBox;
  }
  return AccRole::MenuItem;
}

AccState MenuItem::acc_state() const {
  AccState s = Node::acc_state();
  s.checked = checked_;
  return s;
}

bool MenuItem::AccInvoke() {
  if (separator_) {
    return false;
  }
  Activate();
  return true;
}

bool MenuItem::AccToggle() {
  if (!checkable_ && radio_group_ <= 0) {
    return false;
  }
  Activate();
  return true;
}

std::wstring MenuItem::AccDefaultName() const {
  return text_;
}

}  // namespace auralite::ui
