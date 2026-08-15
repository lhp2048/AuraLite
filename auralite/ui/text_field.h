#pragma once

#include "auralite/ui/node.h"

#include <functional>
#include <string>

namespace auralite::ui {

class TextField : public Node {
 public:
  using ChangeHandler = std::function<void(const std::wstring&)>;

  TextField();

  TextField& text(const std::wstring& t);
  TextField& placeholder(const std::wstring& t);
  TextField& password(bool enabled);
  TextField& font_size(float size);
  TextField& preferred_size(float w, float h);
  TextField& on_change(ChangeHandler handler);

  const std::wstring& text() const { return text_; }
  void set_text(const std::wstring& t);
  bool is_password() const { return password_; }

  void SelectAll();
  void ClearSelection();
  bool HasSelection() const;

  SizeF Measure(float max_w, float max_h) override;
  void Paint(auralite::Canvas& canvas) override;

  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseMove(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;
  void OnChar(wchar_t ch) override;
  void OnFocus() override;
  void OnBlur() override;

  bool WantsIme() const override;
  void OnImeComposition(const std::wstring& composition) override;
  void OnImeResult(const std::wstring& result) override;
  void OnImeEnd() override;

 private:
  static constexpr float kPadX = 10.f;
  static constexpr wchar_t kPasswordChar = L'\x25CF';

  std::wstring DisplayText() const;
  std::wstring VisibleText() const;
  void GetOrderedSelection(size_t* start, size_t* end) const;
  void SetCursor(size_t pos, bool extend_selection);
  size_t HitTestCursor(float window_x) const;
  float TextWidth(const std::wstring& s) const;
  void DeleteSelection();
  void InsertText(const std::wstring& text);
  void DeleteSelectionOrChar(bool forward);
  void NotifyChanged();
  bool CopyToClipboard() const;
  bool PasteFromClipboard();
  bool CutToClipboard();
  void HandleShortcut(UINT vk);

  std::wstring text_;
  std::wstring placeholder_;
  std::wstring composition_;
  bool password_ = false;
  float font_size_ = 15.f;
  size_t sel_start_ = 0;  // anchor
  size_t caret_ = 0;      // active end
  bool selecting_ = false;
  ChangeHandler on_change_;
};

}  // namespace auralite::ui
