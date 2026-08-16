#include "auralite/ui/submenu.h"

#include "auralite/ui/popup_host.h"
#include "auralite/ui/theme.h"
#include "auralite/ui/window.h"

#include <algorithm>
#include <optional>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace auralite::ui {

Submenu::Submenu() {
  hug_width();
  fixed_height(kRowH);
}

Submenu& Submenu::text(const std::wstring& t) {
  text_ = t;
  return *this;
}

Submenu& Submenu::content(std::unique_ptr<Node> node) {
  content_ = std::move(node);
  return *this;
}

Submenu& Submenu::open_on_hover(bool v) {
  open_on_hover_ = v;
  return *this;
}

Submenu& Submenu::bg(const ColorF& c) {
  bg_ = c;
  return *this;
}

Submenu& Submenu::bg_hover(const ColorF& c) {
  bg_hover_ = c;
  return *this;
}

Submenu& Submenu::text_color(const ColorF& c) {
  text_color_ = c;
  return *this;
}

Submenu& Submenu::font_size(float size) {
  font_size_ = size;
  return *this;
}

Submenu& Submenu::corner_radius(float r) {
  corner_radius_ = std::max(0.f, r);
  return *this;
}

void Submenu::RequestRepaint() {
  if (Window* w = host_window()) {
    w->Invalidate();
  }
}

RectF Submenu::AnchorScreenRect() const {
  Window* w = host_window();
  if (!w || !w->hwnd()) {
    return bounds_;
  }
  const float dpi = w->dpi();
  POINT tl{static_cast<LONG>(auralite::PxFromDip(bounds_.x, dpi)),
           static_cast<LONG>(auralite::PxFromDip(bounds_.y, dpi))};
  ClientToScreen(w->hwnd(), &tl);
  return RectF{static_cast<float>(tl.x), static_cast<float>(tl.y),
               auralite::PxFromDip(bounds_.w, dpi),
               auralite::PxFromDip(bounds_.h, dpi)};
}

SizeF Submenu::Measure(float max_w, float max_h) {
  const ThemeTokens& th = Theme::Active();
  const float fs = ResolveFontSize(font_size_);
  const float text_w =
      text_.empty() ? 0.f
                    : auralite::MeasureUiTextWidth(text_, fs, th.font_ui.c_str());
  const float hug_w = kPadX + text_w + kChevronGap + kChevronSlot + kPadX;
  const float hug_h = preferred_height() > 0.f ? preferred_height() : kRowH;
  return ResolveSize(max_w, max_h, hug_w, hug_h);
}

ColorF Submenu::BgColor() const {
  const ThemeTokens& th = Theme::Active();
  if (hovered_) {
    return bg_hover_.value_or(th.accent_soft);
  }
  return bg_.value_or(th.surface);
}

ColorF Submenu::LabelColor() const {
  const ThemeTokens& th = Theme::Active();
  if (text_color_) {
    return *text_color_;
  }
  return th.text;
}

void Submenu::Paint(auralite::Canvas& canvas) {
  if (!visible()) {
    return;
  }
  const ThemeTokens& th = Theme::Active();
  // Layered popups: fully transparent pixels do not receive mouse hits.
  // Always paint an opaque row so the trigger stays clickable.
  const ColorF fill = BgColor();
  if (corner_radius_ > 0.f) {
    canvas.FillRoundedRect(bounds_, corner_radius_, corner_radius_, fill);
  } else {
    canvas.FillRect(bounds_, fill);
  }

  const float fs = ResolveFontSize(font_size_);
  const ColorF label = LabelColor();
  const float chevron_x = bounds_.x + bounds_.w - kPadX - kChevronSlot;
  const RectF label_r{
      bounds_.x + kPadX, bounds_.y,
      std::max(0.f, chevron_x - kChevronGap - (bounds_.x + kPadX)), bounds_.h};
  if (!text_.empty()) {
    canvas.DrawText(text_, label_r, label, fs, th.font_ui.c_str(),
                    auralite::TextHAlign::Left);
  }
  const RectF chevron{chevron_x, bounds_.y, kChevronSlot, bounds_.h};
  canvas.DrawText(L"\u203A", chevron, label, fs, th.font_ui.c_str(),
                  auralite::TextHAlign::Right);
}

void Submenu::OpenIfNeeded() {
  PopupHost* host = PopupHost::Current();
  if (!host || !content_) {
    return;
  }
  Window* win = host_window();
  if (win) {
    if (const std::optional<size_t> level = host->LevelOf(win)) {
      host->DismissFrom(*level + 1);
    }
  }
  if (!content_) {
    return;
  }
  auto leftover =
      host->Push(AnchorScreenRect(), std::move(content_), this);
  if (leftover) {
    content_ = std::move(leftover);
  }
}

void Submenu::OnMouseEnter(const MouseEvent&) {
  hovered_ = true;
  RequestRepaint();
  if (open_on_hover_) {
    OpenIfNeeded();
  }
}

void Submenu::OnMouseLeave(const MouseEvent&) {
  hovered_ = false;
  RequestRepaint();
}

void Submenu::OnMouseUp(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  if (!ContainsPoint(bounds_, e.x, e.y)) {
    return;
  }
  OpenIfNeeded();
}

}  // namespace auralite::ui
