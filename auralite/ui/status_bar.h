#pragma once

#include "auralite/ui/node.h"

#include <string>
#include <vector>

namespace auralite::ui {

// Bottom strip. The first pane fills leftover width; the rest hug from the right.
class StatusBar : public Node {
 public:
  StatusBar();

  StatusBar& items(std::vector<std::wstring> panes);
  StatusBar& add_item(std::wstring text);
  StatusBar& set_item(int index, std::wstring text);
  const std::vector<std::wstring>& items() const { return panes_; }
  int item_count() const { return static_cast<int>(panes_.size()); }

  AccRole acc_role() const override;
  std::wstring AccDefaultName() const override;
  std::wstring AccValue() const override;

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;

 private:
  static constexpr float kBarH = 24.f;
  static constexpr float kPad = 8.f;
  std::vector<std::wstring> panes_;
};

}  // namespace auralite::ui
