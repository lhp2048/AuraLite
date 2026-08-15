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

void ScrollView::set_scroll_offset(float offset) {
  const float old = scroll_offset_;
  scroll_offset_ = offset;
  ClampScrollOffset();
  if (old == scroll_offset_) {
    return;
  }
  // Re-apply content bounds with new offset.
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
    const float cw = std::max(0.f, hug_w - kScrollbarWidth);
    const SizeF cs = c->Measure(cw, 1.0e6f);
    content_h_ = cs.h;
    if (width_policy() != SizePolicy::Fixed) {
      if (content_h_ > hug_h) {
        hug_w = std::max(hug_w, cs.w + kScrollbarWidth);
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

  if (!NeedsScrollbar()) {
    return;
  }
  const RectF track = ScrollbarBounds();
  canvas.FillRect(track, ColorF::FromRgb(220, 220, 220));
  const RectF thumb = ThumbBounds();
  canvas.FillRect(thumb, ColorF::FromRgb(150, 150, 150));
}

Node* ScrollView::HitTest(float x, float y) {
  if (!ContainsPoint(bounds_, x, y)) {
    return nullptr;
  }
  // Scrollbar hits the ScrollView itself (thumb drag).
  if (NeedsScrollbar() && ContainsPoint(ScrollbarBounds(), x, y)) {
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
  // Windows: positive wheel_delta = scroll up (decrease offset).
  const float lines =
      static_cast<float>(e.wheel_delta) / static_cast<float>(WHEEL_DELTA);
  set_scroll_offset(scroll_offset_ - lines * kDefaultLineScroll);
}

void ScrollView::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left || !NeedsScrollbar()) {
    return;
  }
  const RectF thumb = ThumbBounds();
  if (ContainsPoint(thumb, e.x, e.y)) {
    dragging_thumb_ = true;
    drag_thumb_anchor_y_ = e.y;
    drag_scroll_anchor_ = scroll_offset_;
    return;
  }
  const RectF track = ScrollbarBounds();
  if (ContainsPoint(track, e.x, e.y)) {
    set_scroll_offset(ScrollOffsetFromThumbY(e.y - thumb.h * 0.5f));
    dragging_thumb_ = true;
    drag_thumb_anchor_y_ = e.y;
    drag_scroll_anchor_ = scroll_offset_;
  }
}

void ScrollView::OnMouseMove(const MouseEvent& e) {
  if (!dragging_thumb_) {
    return;
  }
  const float delta = e.y - drag_thumb_anchor_y_;
  const float track_h = ViewportHeight();
  const float thumb_h = ThumbBounds().h;
  const float travel = std::max(1.f, track_h - thumb_h);
  const float max_scroll = MaxScrollOffset();
  set_scroll_offset(drag_scroll_anchor_ + max_scroll * delta / travel);
}

void ScrollView::OnMouseUp(const MouseEvent&) {
  dragging_thumb_ = false;
}

float ScrollView::ContentHeight() const {
  return content_h_;
}

float ScrollView::ViewportWidth() const {
  return NeedsScrollbar() ? std::max(0.f, bounds_.w - kScrollbarWidth)
                          : bounds_.w;
}

float ScrollView::ViewportHeight() const {
  return bounds_.h;
}

float ScrollView::MaxScrollOffset() const {
  return std::max(0.f, ContentHeight() - ViewportHeight());
}

bool ScrollView::NeedsScrollbar() const {
  // During Measure, bounds may be empty — use preferred height when set.
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

RectF ScrollView::ScrollbarBounds() const {
  return RectF{bounds_.x + bounds_.w - kScrollbarWidth, bounds_.y,
               kScrollbarWidth, bounds_.h};
}

RectF ScrollView::ThumbBounds() const {
  const float track_h = ViewportHeight();
  const float content_h = std::max(1.f, ContentHeight());
  float thumb_h = track_h * track_h / content_h;
  thumb_h = std::max(kMinThumbHeight, std::min(track_h, thumb_h));
  const float max_scroll = MaxScrollOffset();
  const float travel = std::max(0.f, track_h - thumb_h);
  const float thumb_y =
      (max_scroll > 0.f) ? (travel * scroll_offset_ / max_scroll) : 0.f;
  return RectF{bounds_.x + bounds_.w - kScrollbarWidth, bounds_.y + thumb_y,
               kScrollbarWidth, thumb_h};
}

float ScrollView::ScrollOffsetFromThumbY(float thumb_y) const {
  const float track_h = ViewportHeight();
  const float thumb_h = ThumbBounds().h;
  const float travel = std::max(1.f, track_h - thumb_h);
  const float y =
      std::max(0.f, std::min(thumb_y - bounds_.y, travel));
  return MaxScrollOffset() * y / travel;
}

}  // namespace auralite::ui
