#pragma once

#include "mx/ui/node.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace mx::ui {

// Multiline plain-text editor (no rich text). Soft-wrap by default.
class TextArea : public Node {
 public:
  using ChangeHandler = std::function<void(const std::wstring&)>;

  TextArea();

  TextArea& text(const std::wstring& t);
  TextArea& placeholder(const std::wstring& t);
  TextArea& font_size(float size);
  TextArea& wrap(bool enable);
  bool wrap() const { return wrap_; }
  TextArea& on_change(ChangeHandler handler);

  const std::wstring& text() const { return text_; }
  void set_text(const std::wstring& t);

  AccRole acc_role() const override;
  std::wstring AccValue() const override;
  bool AccSetValue(const std::wstring& value) override;
  std::wstring AccDefaultName() const override;

  SizeF Measure(float max_w, float max_h) override;
  void Layout(const RectF& final_rect) override;
  void Paint(mx::Canvas& canvas) override;

  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseMove(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;
  void OnMouseDoubleClick(const MouseEvent& e) override;
  void OnMouseWheel(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;
  void OnChar(wchar_t ch) override;
  void OnFocus() override;
  void OnBlur() override;

  bool WantsMouseWheel() const override { return true; }
  bool ConsumesEnter() const override { return true; }
  bool WantsIme() const override;
  void OnImeComposition(const std::wstring& composition) override;
  void OnImeResult(const std::wstring& result) override;
  void OnImeEnd() override;

 private:
  static constexpr float kPad = 8.f;

  float LineHeight() const;
  float ContentWidth() const;
  void RebuildLines();
  void EnsureCaretVisible();
  void NotifyChanged();
  void InsertText(const std::wstring& t);
  void DeleteSelection();
  bool HasSelection() const;
  void GetOrderedSelection(size_t* a, size_t* b) const;
  void SetCaret(size_t pos, bool extend);
  size_t HitTestCaret(float x, float y) const;
  void IndexToLineCol(size_t index, int* line, int* col) const;
  size_t LineColToIndex(int line, int col) const;
  bool CopyToClipboard() const;
  bool PasteFromClipboard();
  void HandleShortcut(UINT vk);
  void SelectAll();
  void SelectWordAt(size_t pos);
  void SelectLineAt(size_t pos);
  void AppendWrappedParagraph(size_t para_start, const std::wstring& para,
                              float max_w);

  std::wstring text_;
  std::wstring placeholder_;
  std::wstring composition_;
  std::vector<std::wstring> lines_;
  std::vector<size_t> line_starts_;  // index into text_ for each visual line
  std::optional<float> font_size_;
  float scroll_y_ = 0.f;
  float wrap_width_ = -1.f;
  bool wrap_ = true;
  size_t sel_start_ = 0;
  size_t caret_ = 0;
  bool selecting_ = false;
  bool pending_triple_click_ = false;
  DWORD last_double_click_ms_ = 0;
  ChangeHandler on_change_;
};

}  // namespace mx::ui
