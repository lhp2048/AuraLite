#pragma once

#include "auralite/ui/node.h"

#include <windows.h>

#include <functional>
#include <optional>
#include <string>

namespace auralite::ui {

class TextField : public Node {
 public:
  using ChangeHandler = std::function<void(const std::wstring&)>;

  TextField();
  ~TextField() override;

  TextField& text(const std::wstring& t);
  TextField& placeholder(const std::wstring& t);
  TextField& password(bool enabled);
  TextField& font_size(float size);
  TextField& preferred_size(float w, float h);
  TextField& on_change(ChangeHandler handler);
  TextField& on_submit(std::function<void()> handler);

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
  static constexpr DWORD kCaretBlinkMs = 530;

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
  void ResetCaretBlink();
  void SyncCaretAnim(bool on);
  bool CaretVisible() const;

  std::wstring text_;
  std::wstring placeholder_;
  std::wstring composition_;
  bool password_ = false;
  std::optional<float> font_size_;
  size_t sel_start_ = 0;  // anchor
  size_t caret_ = 0;      // active end
  bool selecting_ = false;
  bool caret_anim_registered_ = false;
  DWORD caret_blink_start_ = 0;
  ChangeHandler on_change_;
  std::function<void()> on_submit_;
};

}  // namespace auralite::ui
