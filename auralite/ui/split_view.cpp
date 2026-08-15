#include "auralite/ui/split_view.h"

#include "auralite/ui/theme.h"

#include <algorithm>
#include <windows.h>

namespace auralite::ui {
namespace {

std::unique_ptr<Node> OrEmpty(std::unique_ptr<Node> node) {
  return node ? std::move(node) : std::make_unique<Node>();
}

}  // namespace

SplitView& SplitView::preferred_size(float w, float h) {
  if (w > 0.f) {
    fixed_width(w);
  } else {
    fill_width();
  }
  if (h > 0.f) {
    fixed_height(h);
  } else {
    fill_height();
  }
  return *this;
}

SplitView& SplitView::set_leading(std::unique_ptr<Node> leading) {
  std::unique_ptr<Node> trail;
  if (children_.size() >= 2) {
    trail = std::move(children_[1]);
  }
  children_.clear();
  AddChild(OrEmpty(std::move(leading)));
  AddChild(OrEmpty(std::move(trail)));
  return *this;
}

SplitView& SplitView::set_trailing(std::unique_ptr<Node> trailing) {
  std::unique_ptr<Node> lead;
  if (!children_.empty()) {
    lead = std::move(children_[0]);
  }
  children_.clear();
  AddChild(OrEmpty(std::move(lead)));
  AddChild(OrEmpty(std::move(trailing)));
  return *this;
}

SplitView& SplitView::set_ratio(float ratio) {
  ratio_ = std::clamp(ratio, 0.f, 1.f);
  return *this;
}

Node* SplitView::leading() const {
  return children_.empty() ? nullptr : children_[0].get();
}

Node* SplitView::trailing() const {
  return children_.size() < 2 ? nullptr : children_[1].get();
}

SizeF SplitView::Measure(float max_w, float max_h) {
  float hug_w = preferred_width() > 0.f ? preferred_width() : max_w;
  float hug_h = preferred_height() > 0.f ? preferred_height() : max_h;

  float child_h = 0.f;
  float child_w = 0.f;
  const float pane_w = std::max(0.f, (hug_w - kDividerSize) * 0.5f);
  if (Node* L = leading()) {
    const SizeF s = L->Measure(pane_w, hug_h);
    child_w += s.w;
    child_h = std::max(child_h, s.h);
  }
  if (Node* R = trailing()) {
    const SizeF s = R->Measure(pane_w, hug_h);
    child_w += s.w;
    child_h = std::max(child_h, s.h);
  }
  if (width_policy() != SizePolicy::Fixed) {
    hug_w = std::max(hug_w, child_w + kDividerSize);
  }
  if (height_policy() != SizePolicy::Fixed) {
    hug_h = std::max(hug_h, child_h);
  }
  return ResolveSize(max_w, max_h, hug_w, hug_h);
}

void SplitView::Layout(const RectF& final_rect) {
  bounds_ = final_rect;
  RelayoutChildren();
}

void SplitView::RelayoutChildren() {
  Node* L = leading();
  Node* R = trailing();
  if (!L || !R) {
    return;
  }

  const float avail = std::max(0.f, bounds_.w - kDividerSize);
  float lead_w = avail * ratio_;
  if (avail >= kMinPane * 2.f) {
    lead_w = std::clamp(lead_w, kMinPane, avail - kMinPane);
  } else {
    lead_w = avail * 0.5f;
  }
  ratio_ = (avail > 0.f) ? (lead_w / avail) : 0.5f;

  const float trail_w = std::max(0.f, avail - lead_w);
  L->Layout(RectF{bounds_.x, bounds_.y, lead_w, bounds_.h});
  R->Layout(RectF{bounds_.x + lead_w + kDividerSize, bounds_.y, trail_w,
                  bounds_.h});
}

RectF SplitView::DividerBounds() const {
  Node* L = leading();
  if (!L) {
    return RectF{};
  }
  return RectF{L->bounds().x + L->bounds().w, bounds_.y, kDividerSize,
               bounds_.h};
}

bool SplitView::IsPointInDivider(float x, float y) const {
  return ContainsPoint(DividerBounds(), x, y);
}

void SplitView::ApplyRatioFromDividerOffset(float offset) {
  const float avail = std::max(0.f, bounds_.w - kDividerSize);
  if (avail <= 0.f) {
    return;
  }
  float lead_w = offset - bounds_.x;
  if (avail >= kMinPane * 2.f) {
    lead_w = std::clamp(lead_w, kMinPane, avail - kMinPane);
  } else {
    lead_w = std::clamp(lead_w, 0.f, avail);
  }
  ratio_ = lead_w / avail;
  RelayoutChildren();
}

void SplitView::Paint(auralite::Canvas& canvas) {
  if (!visible()) {
    return;
  }
  const ThemeTokens& th = Theme::Active();
  if (Node* L = leading()) {
    canvas.FillRect(L->bounds(), th.surface_alt);
    if (L->visible()) {
      L->Paint(canvas);
    }
  }
  if (Node* R = trailing()) {
    canvas.FillRect(R->bounds(), th.surface);
    if (R->visible()) {
      R->Paint(canvas);
    }
  }
  const RectF div = DividerBounds();
  canvas.FillRect(div, th.border);
  const float mid = div.x + div.w * 0.5f;
  canvas.FillRect(RectF{mid - 0.5f, div.y + div.h * 0.35f, 1.f, div.h * 0.3f},
                  th.text_muted);
}

Node* SplitView::HitTest(float x, float y) {
  if (!visible() || !ContainsPoint(bounds_, x, y)) {
    return nullptr;
  }
  if (IsPointInDivider(x, y)) {
    return this;
  }
  if (Node* L = leading()) {
    if (Node* hit = L->HitTest(x, y)) {
      return hit;
    }
  }
  if (Node* R = trailing()) {
    if (Node* hit = R->HitTest(x, y)) {
      return hit;
    }
  }
  return this;
}

void SplitView::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left || !IsPointInDivider(e.x, e.y)) {
    return;
  }
  dragging_ = true;
  SetCursor(LoadCursorW(nullptr, IDC_SIZEWE));
}

void SplitView::OnMouseMove(const MouseEvent& e) {
  if (IsPointInDivider(e.x, e.y) || dragging_) {
    SetCursor(LoadCursorW(nullptr, IDC_SIZEWE));
  }
  if (!dragging_) {
    return;
  }
  ApplyRatioFromDividerOffset(e.x);
}

void SplitView::OnMouseUp(const MouseEvent& e) {
  (void)e;
  dragging_ = false;
}

}  // namespace auralite::ui
