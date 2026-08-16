#pragma once

#include "auralite/ui/row.h"

namespace auralite::ui {

// Horizontal caption strip. Child layout is Row (generic YAML / DSL).
// Clicking empty space or non-focusable children (Label) drags the host HWND;
// focusable children (Button, TextField, …) keep their own mouse handling.
class TitleBar : public Row {
 public:
  TitleBar();

  Node* HitTest(float x, float y) override;
  void OnMouseDown(const MouseEvent& e) override;
};

}  // namespace auralite::ui
