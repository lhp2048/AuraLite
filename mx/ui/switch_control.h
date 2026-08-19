#pragma once

#include "mx/ui/anim.h"
#include "mx/ui/node.h"

#include <functional>
#include <optional>
#include <string>

namespace mx::ui {

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

  AccRole acc_role() const override;
  AccState acc_state() const override;
  bool AccToggle() override;
  std::wstring AccDefaultName() const override;

  SizeF Measure(float max_w, float max_h) override;
  void Paint(mx::Canvas& canvas) override;

  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;

  float thumb_t() const { return thumb_t_; }

 private:
  void Toggle();
  RectF TrackRect() const;
  void SyncThumb(bool instant);
  void OnAnimateChanged() override;
  void OnHostWindowChanged() override;

  static constexpr float kTrackWidth = 40.f;
  static constexpr float kTrackHeight = 20.f;
  static constexpr float kThumbSize = 16.f;
  static constexpr float kLabelGap = 8.f;

  std::wstring text_;
  std::optional<float> font_size_;
  bool on_ = false;
  float thumb_t_ = 0.f;
  Tween thumb_tween_;
  ChangedHandler on_changed_;
  bool pressed_ = false;
};

}  // namespace mx::ui
