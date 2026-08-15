#include "auralite/ui/tab.h"

#include "auralite/ui/theme.h"

#include <algorithm>

namespace auralite::ui {

Tab::Tab() {
  fill_width();
  fill_height();
}

int Tab::page_count() const {
  int n = 0;
  for (const auto& c : children_) {
    if (c) {
      ++n;
    }
  }
  return n;
}

Node* Tab::SelectedPage() const {
  int i = 0;
  for (const auto& c : children_) {
    if (!c) {
      continue;
    }
    if (i == selected_) {
      return c.get();
    }
    ++i;
  }
  return nullptr;
}

Tab& Tab::set_selected(int index) {
  const int n = page_count();
  if (n <= 0) {
    selected_ = 0;
    return *this;
  }
  selected_ = std::clamp(index, 0, n - 1);
  return *this;
}

Tab& Tab::set_headers(std::vector<std::wstring> titles) {
  headers_ = std::move(titles);
  return *this;
}

Tab& Tab::add_header(std::wstring title) {
  headers_.push_back(std::move(title));
  return *this;
}

Tab& Tab::header_height(float h) {
  header_height_ = std::max(0.f, h);
  return *this;
}

Tab& Tab::on_selected(SelectedHandler handler) {
  on_selected_ = std::move(handler);
  return *this;
}

float Tab::HeaderH() const {
  return has_headers() ? header_height_ : 0.f;
}

RectF Tab::PageRect() const {
  const float hh = HeaderH();
  return RectF{bounds_.x, bounds_.y + hh, bounds_.w,
               std::max(0.f, bounds_.h - hh)};
}

int Tab::HeaderIndexAt(float x, float y) const {
  if (!has_headers() || !ContainsPoint(header_bounds_, x, y)) {
    return -1;
  }
  const int n = static_cast<int>(headers_.size());
  if (n <= 0 || header_bounds_.w <= 0.f) {
    return -1;
  }
  const float slot = header_bounds_.w / static_cast<float>(n);
  const int idx =
      static_cast<int>((x - header_bounds_.x) / std::max(1.f, slot));
  return std::clamp(idx, 0, n - 1);
}

SizeF Tab::Measure(float max_w, float max_h) {
  const float hh = HeaderH();
  const float page_max_h = std::max(0.f, max_h - hh);
  float hug_w = 0.f;
  float hug_h = 0.f;
  for (const auto& c : children_) {
    if (!c) {
      continue;
    }
    const SizeF s = c->Measure(max_w, page_max_h);
    hug_w = std::max(hug_w, s.w);
    hug_h = std::max(hug_h, s.h);
  }
  if (has_headers()) {
    hug_h += hh;
  }
  return ResolveSize(max_w, max_h, hug_w, hug_h);
}

void Tab::Layout(const RectF& final_rect) {
  bounds_ = final_rect;
  const float hh = HeaderH();
  header_bounds_ = has_headers()
                       ? RectF{final_rect.x, final_rect.y, final_rect.w, hh}
                       : RectF{};
  const RectF page = PageRect();
  for (auto& c : children_) {
    if (c) {
      c->Layout(page);
    }
  }
}

void Tab::Paint(auralite::Canvas& canvas) {
  if (has_headers()) {
    const ThemeTokens& th = Theme::Active();
    const int n = static_cast<int>(headers_.size());
    const float slot =
        n > 0 ? header_bounds_.w / static_cast<float>(n) : header_bounds_.w;
    canvas.FillRect(header_bounds_, th.surface_alt);
    for (int i = 0; i < n; ++i) {
      const RectF cell{header_bounds_.x + slot * static_cast<float>(i),
                       header_bounds_.y, slot, header_bounds_.h};
      if (i == selected_) {
        canvas.FillRect(cell, th.accent_soft);
      }
      canvas.DrawText(headers_[static_cast<size_t>(i)], cell, th.text,
                      th.font_size, th.font_ui.c_str(),
                      auralite::TextHAlign::Center);
      if (i + 1 < n) {
        canvas.FillRect(RectF{cell.x + cell.w - 1.f, cell.y + 6.f, 1.f,
                              cell.h - 12.f},
                        th.divider);
      }
    }
    canvas.FillRect(RectF{header_bounds_.x, header_bounds_.y + header_bounds_.h - 1.f,
                          header_bounds_.w, 1.f},
                    th.border);
  }
  if (Node* page = SelectedPage()) {
    page->Paint(canvas);
  }
}

Node* Tab::HitTest(float x, float y) {
  if (!ContainsPoint(bounds_, x, y)) {
    return nullptr;
  }
  if (has_headers() && ContainsPoint(header_bounds_, x, y)) {
    return this;
  }
  if (Node* page = SelectedPage()) {
    if (Node* hit = page->HitTest(x, y)) {
      return hit;
    }
  }
  return this;
}

void Tab::OnMouseDown(const MouseEvent& e) {
  const int idx = HeaderIndexAt(e.x, e.y);
  if (idx >= 0 && idx != selected_) {
    set_selected(idx);
    if (on_selected_) {
      on_selected_(selected_);
    }
  }
}

}  // namespace auralite::ui
