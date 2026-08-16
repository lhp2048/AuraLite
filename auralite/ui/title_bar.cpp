#include "auralite/ui/title_bar.h"

#include "auralite/ui/window.h"

namespace auralite::ui {

TitleBar::TitleBar() {
  v_align(Align::Center);
  fixed_height(36.f);
}

Node* TitleBar::HitTest(float x, float y) {
  Node* hit = Row::HitTest(x, y);
  if (!hit) {
    return nullptr;
  }
  for (Node* n = hit; n && n != this; n = n->parent()) {
    if (n->focusable()) {
      return hit;
    }
  }
  return this;
}

void TitleBar::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  if (Window* w = host_window()) {
    w->BeginCaptionDrag();
  }
}

}  // namespace auralite::ui
