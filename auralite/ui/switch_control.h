#pragma once

#include "auralite/ui/node.h"

#include <functional>
#include <string>

namespace auralite::ui {

class Switch : public Node {
 public:
  using ChangedHandler = std::function<void(bool on)>;

  Switch();

  Switch& text(const std::wstring& t);
  Switch& font_size(float size);
  Switch& on(bool v);
  Switch& on_changed(ChangedHandler handler);

  const std::wstring& text() const { return text_; }
  bool is_on() const { return on_; }

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;

  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;

 private:
  void Toggle();
  RectF TrackRect() const;

  static constexpr float kTrackWidth = 40.f;
  static constexpr float kTrackHeight = 20.f;
  static constexpr float kThumbSize = 16.f;
  static constexpr float kLabelGap = 8.f;

  std::wstring text_;
  float font_size_ = 14.f;
  bool on_ = false;
  ChangedHandler on_changed_;
  bool pressed_ = false;
};

}  // namespace auralite::ui
