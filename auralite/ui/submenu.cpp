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
  POINT tl{static_cast<LONG>(bounds_.x), static_cast<LONG>(bounds_.y)};
  ClientToScreen(w->hwnd(), &tl);
  return RectF{static_cast<float>(tl.x), static_cast<float>(tl.y), bounds_.w,
               bounds_.h};
}

SizeF Submenu::Measure(float max_w, float max_h) {
  const ThemeTokens& th = Theme::Active();
  const float fs = ResolveFontSize(std::nullopt);
  const float text_w =
      text_.empty() ? 0.f
                    : auralite::MeasureUiTextWidth(text_, fs, th.font_ui.c_str());
  const float hug_w = kPadX + text_w + kChevronGap + kChevronSlot + kPadX;
  return ResolveSize(max_w, max_h, hug_w, kRowH);
}

void Submenu::Paint(auralite::Canvas& canvas) {
  if (!visible()) {
    return;
  }
  const ThemeTokens& th = Theme::Active();
  if (hovered_) {
    canvas.FillRect(bounds_, th.accent_soft);
  }

  const float fs = ResolveFontSize(std::nullopt);
  const float chevron_x = bounds_.x + bounds_.w - kPadX - kChevronSlot;
  const RectF label{bounds_.x + kPadX, bounds_.y,
                    std::max(0.f, chevron_x - kChevronGap - (bounds_.x + kPadX)),
                    bounds_.h};
  if (!text_.empty()) {
    canvas.DrawText(text_, label, th.text, fs, th.font_ui.c_str(),
                    auralite::TextHAlign::Left);
  }
  const RectF chevron{chevron_x, bounds_.y, kChevronSlot, bounds_.h};
  canvas.DrawText(L"\u203A", chevron, th.text, fs, th.font_ui.c_str(),
                  auralite::TextHAlign::Right);
}

void Submenu::OpenIfNeeded() {
  PopupHost* host = PopupHost::Current();
  if (!host || !content_) {
    return;
  }
  Window* win = host_window();
  if (win) {
    const size_t level = host->LevelOf(win);
    host->DismissFrom(level + 1);
  }
  if (!content_) {
    return;
  }
  host->Push(AnchorScreenRect(), std::move(content_), this);
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
