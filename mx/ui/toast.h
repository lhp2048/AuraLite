#pragma once

#include "mx/ui/node.h"

#include <functional>
#include <optional>
#include <string>

namespace mx::ui {

enum class ToastVariant { Info, Success, Danger };

// In-tree banner or payload for Window::ShowToast (overlay). Same Node class.
class Toast : public Node {
 public:
  using DismissHandler = std::function<void()>;

  Toast();

  Toast& text(const std::wstring& t);
  Toast& variant(ToastVariant v);
  // Seconds on overlay; <=0 means stay until click. Ignored in-tree.
  Toast& duration_sec(float s);
  Toast& animate(bool on);
  Toast& fade_sec(float s);
  Toast& font_size(float size);
  Toast& on_dismiss(DismissHandler handler);

  const std::wstring& text() const { return text_; }
  ToastVariant variant() const { return variant_; }
  float duration_sec() const { return duration_sec_; }
  bool animate() const { return Node::animate(); }
  float fade_sec() const { return fade_sec_; }
  float effective_fade_sec() const {
    return Node::animate() && fade_sec_ > 0.f ? fade_sec_ : 0.f;
  }

  AccRole acc_role() const override;
  SizeF Measure(float max_w, float max_h) override;
  void Paint(mx::Canvas& canvas) override;
  void OnMouseDown(const MouseEvent& e) override;
  bool AccInvoke() override;
  std::wstring AccDefaultName() const override;

 private:
  std::wstring text_;
  ToastVariant variant_ = ToastVariant::Info;
  float duration_sec_ = 2.5f;
  float fade_sec_ = 0.2f;
  std::optional<float> font_size_;
  DismissHandler on_dismiss_;
};

}  // namespace mx::ui
