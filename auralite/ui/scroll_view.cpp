#include "auralite/ui/scroll_view.h"

#include <algorithm>

namespace auralite::ui {

ScrollView& ScrollView::preferred_size(float w, float h) {
  if (w > 0.f) {
    fixed_width(w);
  } else {
    fill_width();
  }
  if (h > 0.f) {
    fixed_height(h);
  } else {
    hug_height();
  }
  return *this;
}

ScrollView& ScrollView::set_content(std::unique_ptr<Node> content) {
  children_.clear();
  scroll_offset_ = 0.f;
  if (content) {
    AddChild(std::move(content));
  }
  return *this;
}

Node* ScrollView::content() const {
  return children_.empty() ? nullptr : children_.front().get();
}

void ScrollView::SyncVScrollBar() {
  vscroll_.set_content_height(content_h_);
  vscroll_.set_viewport_height(ViewportHeight());
  vscroll_.set_scroll_offset(scroll_offset_);
  vscroll_.set_track_bounds(RectF{
      bounds_.x + bounds_.w - VerticalScrollbar::kWidth, bounds_.y,
      VerticalScrollbar::kWidth, ViewportHeight()});
  scroll_offset_ = vscroll_.scroll_offset();
}

void ScrollView::set_scroll_offset(float offset) {
  const float old = scroll_offset_;
  scroll_offset_ = offset;
  ClampScrollOffset();
  if (old == scroll_offset_) {
    return;
  }
  if (Node* c = content()) {
    const float cw = ViewportWidth();
    const float ch = std::max(content_h_, ViewportHeight());
    c->Layout(RectF{bounds_.x, bounds_.y - scroll_offset_, cw, ch});
  }
}

SizeF ScrollView::Measure(float max_w, float max_h) {
  float hug_w = preferred_width() > 0.f ? preferred_width() : max_w;
  float hug_h = preferred_height() > 0.f ? preferred_height()
                                         : std::min(200.f, max_h);

  if (Node* c = content()) {
    const float cw = std::max(0.f, hug_w - VerticalScrollbar::kWidth);
    const SizeF cs = c->Measure(cw, 1.0e6f);
    content_h_ = cs.h;
    if (width_policy() != SizePolicy::Fixed) {
      if (content_h_ > hug_h) {
        hug_w = std::max(hug_w, cs.w + VerticalScrollbar::kWidth);
      } else {
        hug_w = std::max(hug_w, cs.w);
      }
    }
  } else {
    content_h_ = 0.f;
  }
  return ResolveSize(max_w, max_h, hug_w, hug_h);
}

void ScrollView::Layout(const RectF& final_rect) {
  bounds_ = final_rect;
  if (Node* c = content()) {
    const float cw = ViewportWidth();
    const SizeF cs = c->Measure(cw, 1.0e6f);
    content_h_ = cs.h;
    ClampScrollOffset();
    const float ch = std::max(content_h_, ViewportHeight());
    c->Layout(RectF{bounds_.x, bounds_.y - scroll_offset_, cw, ch});
  } else {
    content_h_ = 0.f;
    scroll_offset_ = 0.f;
  }
}

void ScrollView::Paint(auralite::Canvas& canvas) {
  const RectF clip = ViewportRect();
  canvas.PushAxisAlignedClip(clip);
  if (Node* c = content()) {
    c->Paint(canvas);
  }
  canvas.PopAxisAlignedClip();

  SyncVScrollBar();
  vscroll_.Paint(canvas);
}

Node* ScrollView::HitTest(float x, float y) {
  if (!ContainsPoint(bounds_, x, y)) {
    return nullptr;
  }
  SyncVScrollBar();
  if (vscroll_.needed() && ContainsPoint(vscroll_.track_bounds(), x, y)) {
    return this;
  }
  if (Node* c = content()) {
    if (Node* hit = c->HitTest(x, y)) {
      return hit;
    }
  }
  return this;
}

void ScrollView::OnMouseWheel(const MouseEvent& e) {
  if (e.wheel_delta == 0 || !NeedsScrollbar()) {
    return;
  }
  const float lines =
      static_cast<float>(e.wheel_delta) / static_cast<float>(WHEEL_DELTA);
  set_scroll_offset(scroll_offset_ - lines * kDefaultLineScroll);
}

void ScrollView::OnMouseDown(const MouseEvent& e) {
  SyncVScrollBar();
  if (vscroll_.OnMouseDown(e)) {
    set_scroll_offset(vscroll_.scroll_offset());
  }
}

void ScrollView::OnMouseMove(const MouseEvent& e) {
  if (vscroll_.OnMouseMove(e)) {
    set_scroll_offset(vscroll_.scroll_offset());
  }
}

void ScrollView::OnMouseUp(const MouseEvent& e) { vscroll_.OnMouseUp(e); }

float ScrollView::ContentHeight() const { return content_h_; }

float ScrollView::ViewportWidth() const {
  return NeedsScrollbar()
             ? std::max(0.f, bounds_.w - VerticalScrollbar::kWidth)
             : bounds_.w;
}

float ScrollView::ViewportHeight() const { return bounds_.h; }

float ScrollView::MaxScrollOffset() const {
  return std::max(0.f, ContentHeight() - ViewportHeight());
}

bool ScrollView::NeedsScrollbar() const {
  const float vh = bounds_.h > 0.f
                       ? bounds_.h
                       : (preferred_height() > 0.f ? preferred_height() : 0.f);
  return content_h_ > vh && vh > 0.f;
}

void ScrollView::ClampScrollOffset() {
  scroll_offset_ =
      std::max(0.f, std::min(scroll_offset_, MaxScrollOffset()));
}

RectF ScrollView::ViewportRect() const {
  return RectF{bounds_.x, bounds_.y, ViewportWidth(), ViewportHeight()};
}

}  // namespace auralite::ui
