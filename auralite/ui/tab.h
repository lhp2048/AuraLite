#pragma once

#include "auralite/ui/anim.h"
#include "auralite/ui/node.h"

#include <functional>
#include <string>
#include <vector>

namespace auralite::ui {

// Stacked pages; optional header strip (titles) to switch selected page.
class Tab : public Node {
 public:
  Tab();

  Tab& set_selected(int index);
  int selected() const { return selected_; }
  float visual_selected() const { return visual_selected_; }
  int page_count() const;

  Tab& set_headers(std::vector<std::wstring> titles);
  Tab& add_header(std::wstring title);
  Tab& header_height(float h);
  const std::vector<std::wstring>& headers() const { return headers_; }
  bool has_headers() const { return !headers_.empty(); }

  using SelectedHandler = std::function<void(int index)>;
  Tab& on_selected(SelectedHandler handler);

  SizeF Measure(float max_w, float max_h) override;
  void Layout(const RectF& final_rect) override;
  void Paint(auralite::Canvas& canvas) override;
  Node* HitTest(float x, float y) override;
  void OnMouseDown(const MouseEvent& e) override;

 private:
  Node* SelectedPage() const;
  float HeaderH() const;
  RectF PageRect() const;
  int HeaderIndexAt(float x, float y) const;
  void SyncPageVisibility();
  void SyncIndicator(bool instant);
  void OnAnimateChanged() override;
  void OnHostWindowChanged() override;

  int selected_ = 0;
  float visual_selected_ = 0.f;
  Tween indicator_tween_;
  std::vector<std::wstring> headers_;
  float header_height_ = 36.f;
  RectF header_bounds_{};
  SelectedHandler on_selected_;
};

}  // namespace auralite::ui
