#pragma once

#include "auralite/ui/node.h"

#include <functional>
#include <optional>
#include <string>

namespace auralite::ui {

class Radio : public Node {
 public:
  using ChangedHandler = std::function<void(bool checked)>;

  Radio();

  Radio& text(const std::wstring& t);
  Radio& font_size(float size);
  Radio& group_id(int id);
  Radio& checked(bool v);
  Radio& on_changed(ChangedHandler handler);

  const std::wstring& text() const { return text_; }
  int group_id() const { return group_id_; }
  bool checked() const { return checked_; }

  AccRole acc_role() const override;
  AccState acc_state() const override;
  bool AccInvoke() override;
  bool AccToggle() override;
  std::wstring AccDefaultName() const override;

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;

  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;

 private:
  void Select();
  void UncheckGroupPeers();
  void SetCheckedInternal(bool v, bool notify);
  RectF DotRect() const;

  static void UncheckRadiosInTree(Node* node, Radio* except, int group);

  static constexpr float kDotSize = 16.f;
  static constexpr float kLabelGap = 8.f;

  std::wstring text_;
  std::optional<float> font_size_;
  int group_id_ = 0;
  bool checked_ = false;
  ChangedHandler on_changed_;
  bool pressed_ = false;
};

}  // namespace auralite::ui
