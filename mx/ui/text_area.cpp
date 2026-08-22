#include "mx/ui/text_area.h"

#include "mx/ui/theme.h"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cwctype>

namespace mx::ui {
namespace {

bool IsWordChar(wchar_t c) {
  if (c == L'_') {
    return true;
  }
  if (iswalnum(static_cast<wint_t>(c))) {
    return true;
  }
  if (c >= 0x3400 && c <= 0x9FFF) {
    return true;
  }
  if (c >= 0xF900 && c <= 0xFAFF) {
    return true;
  }
  if (c >= 0x3040 && c <= 0x30FF) {
    return true;
  }
  if (c >= 0xAC00 && c <= 0xD7AF) {
    return true;
  }
  return false;
}

void ExpandWordRange(const std::wstring& text, size_t pos, size_t* out_a,
                     size_t* out_b) {
  if (!out_a || !out_b) {
    return;
  }
  *out_a = 0;
  *out_b = 0;
  if (text.empty()) {
    return;
  }
  if (pos > text.size()) {
    pos = text.size();
  }
  if (pos == text.size()) {
    --pos;
  }
  if (iswspace(static_cast<wint_t>(text[pos]))) {
    *out_a = pos;
    *out_b = pos;
    return;
  }
  if (!IsWordChar(text[pos])) {
    size_t a = pos;
    size_t b = pos + 1;
    while (a > 0 && !IsWordChar(text[a - 1]) &&
           !iswspace(static_cast<wint_t>(text[a - 1]))) {
      --a;
    }
    while (b < text.size() && !IsWordChar(text[b]) &&
           !iswspace(static_cast<wint_t>(text[b]))) {
      ++b;
    }
    *out_a = a;
    *out_b = b;
    return;
  }
  size_t a = pos;
  while (a > 0 && IsWordChar(text[a - 1])) {
    --a;
  }
  size_t b = pos + 1;
  while (b < text.size() && IsWordChar(text[b])) {
    ++b;
  }
  *out_a = a;
  *out_b = b;
}

}  // namespace

TextArea::TextArea() {
  set_focusable(true);
  fill_width();
  fixed_height(120.f);
  RebuildLines();
}

TextArea& TextArea::text(const std::wstring& t) {
  set_text(t);
  return *this;
}

TextArea& TextArea::placeholder(const std::wstring& t) {
  placeholder_ = t;
  return *this;
}

TextArea& TextArea::font_size(float size) {
  font_size_ = size;
  RebuildLines();
  return *this;
}

TextArea& TextArea::wrap(bool enable) {
  wrap_ = enable;
  wrap_width_ = -1.f;
  RebuildLines();
  return *this;
}

TextArea& TextArea::on_change(ChangeHandler handler) {
  on_change_ = std::move(handler);
  return *this;
}

void TextArea::set_text(const std::wstring& t) {
  if (text_ == t) {
    return;
  }
  text_ = t;
  caret_ = text_.size();
  sel_start_ = caret_;
  composition_.clear();
  RebuildLines();
  EnsureCaretVisible();
  NotifyChanged();
}

float TextArea::LineHeight() const { return ResolveFontSize(font_size_) + 6.f; }

float TextArea::ContentWidth() const {
  return std::max(1.f, bounds_.w - kPad * 2.f);
}

void TextArea::AppendWrappedParagraph(size_t para_start,
                                      const std::wstring& para, float max_w) {
  if (para.empty()) {
    lines_.push_back(L"");
    line_starts_.push_back(para_start);
    return;
  }
  if (!wrap_ || max_w <= 1.f) {
    lines_.push_back(para);
    line_starts_.push_back(para_start);
    return;
  }

  const float fs = ResolveFontSize(font_size_);
  size_t i = 0;
  while (i < para.size()) {
    size_t end = i + 1;
    size_t last_fit = i + 1;
    while (end <= para.size()) {
      const float w = mx::MeasureUiTextWidth(
          para.substr(i, end - i), fs, Theme::Active().font_ui.c_str());
      if (w <= max_w) {
        last_fit = end;
        if (end == para.size()) {
          break;
        }
        ++end;
      } else {
        break;
      }
    }
    if (last_fit == i) {
      last_fit = i + 1;
    }
    lines_.push_back(para.substr(i, last_fit - i));
    line_starts_.push_back(para_start + i);
    i = last_fit;
  }
}

void TextArea::RebuildLines() {
  const float max_w = (bounds_.w > 0.f) ? ContentWidth() : 1.0e9f;
  wrap_width_ = max_w;

  lines_.clear();
  line_starts_.clear();

  size_t start = 0;
  for (size_t i = 0; i < text_.size(); ++i) {
    if (text_[i] == L'\n') {
      AppendWrappedParagraph(start, text_.substr(start, i - start), max_w);
      start = i + 1;
    }
  }
  AppendWrappedParagraph(start, text_.substr(start), max_w);

  if (lines_.empty()) {
    lines_.push_back(L"");
    line_starts_.assign(1, 0);
  }
}

void TextArea::Layout(const RectF& final_rect) {
  Node::Layout(final_rect);
  const float w = ContentWidth();
  if (std::fabs(w - wrap_width_) > 0.5f) {
    RebuildLines();
    EnsureCaretVisible();
  }
}

void TextArea::EnsureCaretVisible() {
  int line = 0;
  int col = 0;
  IndexToLineCol(caret_, &line, &col);
  const float lh = LineHeight();
  const float view_h = std::max(0.f, bounds_.h - kPad * 2.f);
  const float y = static_cast<float>(line) * lh;
  if (y < scroll_y_) {
    scroll_y_ = y;
  } else if (y + lh > scroll_y_ + view_h) {
    scroll_y_ = y + lh - view_h;
  }
  scroll_y_ = std::max(0.f, scroll_y_);
}

void TextArea::NotifyChanged() {
  NotifyAccValueChanged();
  if (on_change_) {
    on_change_(text_);
  }
}

bool TextArea::HasSelection() const { return sel_start_ != caret_; }

void TextArea::GetOrderedSelection(size_t* a, size_t* b) const {
  *a = (std::min)(sel_start_, caret_);
  *b = (std::max)(sel_start_, caret_);
}

void TextArea::SetCaret(size_t pos, bool extend) {
  caret_ = (std::min)(pos, text_.size());
  if (!extend) {
    sel_start_ = caret_;
  }
  EnsureCaretVisible();
}

void TextArea::IndexToLineCol(size_t index, int* line, int* col) const {
  index = (std::min)(index, text_.size());
  int l = static_cast<int>(line_starts_.size()) - 1;
  for (int i = 0; i < static_cast<int>(line_starts_.size()); ++i) {
    if (line_starts_[static_cast<size_t>(i)] > index) {
      l = i - 1;
      break;
    }
    l = i;
  }
  l = (std::max)(0, l);
  // Prefer end of previous visual line when index lands on a soft-wrap boundary
  // that is also the start of the next line (except after hard '\n').
  if (l + 1 < static_cast<int>(line_starts_.size()) &&
      line_starts_[static_cast<size_t>(l + 1)] == index && index > 0 &&
      text_[index - 1] != L'\n') {
    // Stay on current line start (beginning of wrapped line) — OK for caret
    // at wrap point showing at start of next line (common editors).
  }
  *line = l;
  *col = static_cast<int>(index - line_starts_[static_cast<size_t>(l)]);
  const int max_col =
      static_cast<int>(lines_[static_cast<size_t>(l)].size());
  if (*col > max_col) {
    *col = max_col;
  }
}

size_t TextArea::LineColToIndex(int line, int col) const {
  if (lines_.empty()) {
    return 0;
  }
  line = std::clamp(line, 0, static_cast<int>(lines_.size()) - 1);
  const std::wstring& s = lines_[static_cast<size_t>(line)];
  col = std::clamp(col, 0, static_cast<int>(s.size()));
  return line_starts_[static_cast<size_t>(line)] + static_cast<size_t>(col);
}

size_t TextArea::HitTestCaret(float x, float y) const {
  const float content_x = bounds_.x + kPad;
  const float content_y = bounds_.y + kPad - scroll_y_;
  const float lh = LineHeight();
  int line = static_cast<int>((y - content_y) / lh);
  line = std::clamp(line, 0, static_cast<int>(lines_.size()) - 1);
  const std::wstring& s = lines_[static_cast<size_t>(line)];
  const float fs = ResolveFontSize(font_size_);
  float best_d = 1.0e9f;
  size_t best = 0;
  for (size_t c = 0; c <= s.size(); ++c) {
    const float tw = mx::MeasureUiTextWidth(
        s.substr(0, c), fs, Theme::Active().font_ui.c_str());
    const float d = std::fabs((content_x + tw) - x);
    if (d < best_d) {
      best_d = d;
      best = c;
    }
  }
  return LineColToIndex(line, static_cast<int>(best));
}

void TextArea::DeleteSelection() {
  if (!HasSelection()) {
    return;
  }
  size_t a = 0;
  size_t b = 0;
  GetOrderedSelection(&a, &b);
  text_.erase(a, b - a);
  caret_ = a;
  sel_start_ = a;
  RebuildLines();
  NotifyChanged();
}

void TextArea::InsertText(const std::wstring& t) {
  if (t.empty()) {
    return;
  }
  if (HasSelection()) {
    DeleteSelection();
  }
  text_.insert(caret_, t);
  caret_ += t.size();
  sel_start_ = caret_;
  RebuildLines();
  EnsureCaretVisible();
  NotifyChanged();
}

bool TextArea::CopyToClipboard() const {
  if (!HasSelection()) {
    return false;
  }
  size_t a = 0;
  size_t b = 0;
  GetOrderedSelection(&a, &b);
  const std::wstring selected = text_.substr(a, b - a);
  if (!OpenClipboard(nullptr)) {
    return false;
  }
  EmptyClipboard();
  const size_t bytes = (selected.size() + 1) * sizeof(wchar_t);
  HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
  if (!mem) {
    CloseClipboard();
    return false;
  }
  void* locked = GlobalLock(mem);
  if (!locked) {
    GlobalFree(mem);
    CloseClipboard();
    return false;
  }
  std::memcpy(locked, selected.c_str(), bytes);
  GlobalUnlock(mem);
  const bool ok = SetClipboardData(CF_UNICODETEXT, mem) != nullptr;
  if (!ok) {
    GlobalFree(mem);
  }
  CloseClipboard();
  return ok;
}

bool TextArea::PasteFromClipboard() {
  if (!IsClipboardFormatAvailable(CF_UNICODETEXT) || !OpenClipboard(nullptr)) {
    return false;
  }
  HANDLE data = GetClipboardData(CF_UNICODETEXT);
  bool ok = false;
  if (data) {
    const wchar_t* clip = static_cast<const wchar_t*>(GlobalLock(data));
    if (clip) {
      std::wstring paste(clip);
      GlobalUnlock(data);
      std::wstring filtered;
      filtered.reserve(paste.size());
      for (wchar_t ch : paste) {
        if (ch == L'\r') {
          continue;
        }
        filtered.push_back(ch);
      }
      InsertText(filtered);
      ok = true;
    }
  }
  CloseClipboard();
  return ok;
}

void TextArea::HandleShortcut(UINT vk) {
  switch (vk) {
    case 'A':
      SelectAll();
      break;
    case 'C':
      CopyToClipboard();
      break;
    case 'X':
      if (CopyToClipboard()) {
        DeleteSelection();
      }
      break;
    case 'V':
      PasteFromClipboard();
      break;
    default:
      break;
  }
}

void TextArea::SelectAll() {
  sel_start_ = 0;
  caret_ = text_.size();
}

void TextArea::SelectWordAt(size_t pos) {
  size_t a = 0;
  size_t b = 0;
  ExpandWordRange(text_, pos, &a, &b);
  if (a == b) {
    SetCaret(a, false);
    return;
  }
  sel_start_ = a;
  caret_ = b;
}

void TextArea::SelectLineAt(size_t pos) {
  if (lines_.empty() || line_starts_.empty()) {
    SelectAll();
    return;
  }
  int line = 0;
  int col = 0;
  IndexToLineCol(pos, &line, &col);
  const size_t a = line_starts_[static_cast<size_t>(line)];
  size_t b = text_.size();
  if (static_cast<size_t>(line) + 1 < line_starts_.size()) {
    b = line_starts_[static_cast<size_t>(line) + 1];
    // Prefer excluding the trailing newline from selection when present.
    if (b > a && text_[b - 1] == L'\n') {
      --b;
    }
  }
  sel_start_ = a;
  caret_ = b;
  EnsureCaretVisible();
}

SizeF TextArea::Measure(float max_w, float max_h) {
  const float hug_h =
      preferred_height() > 0.f ? preferred_height() : 120.f;
  return ResolveSize(max_w, max_h,
                     preferred_width() > 0.f ? preferred_width() : max_w, hug_h);
}

void TextArea::Paint(mx::Canvas& canvas) {
  if (!visible()) {
    return;
  }
  const ThemeTokens& th = Theme::Active();
  const float fs = ResolveFontSize(font_size_);
  canvas.FillRoundedRect(bounds_, 6.f, 6.f, th.surface);
  canvas.DrawRect(bounds_, focused() ? th.border_focus : th.border, 1.5f);

  canvas.PushAxisAlignedClip(bounds_);

  const float content_x = bounds_.x + kPad;
  const float content_y = bounds_.y + kPad - scroll_y_;
  const float lh = LineHeight();
  const float content_w = std::max(0.f, bounds_.w - kPad * 2.f);
  const wchar_t* font = th.font_ui.c_str();

  const bool show_ph =
      text_.empty() && composition_.empty() && !placeholder_.empty();
  if (show_ph) {
    canvas.DrawText(placeholder_,
                    RectF{content_x, bounds_.y + kPad, content_w, lh},
                    th.text_muted, fs, font, mx::TextHAlign::Left);
  } else {
    size_t sel_a = 0;
    size_t sel_b = 0;
    if (focused() && HasSelection()) {
      GetOrderedSelection(&sel_a, &sel_b);
    }
    for (int i = 0; i < static_cast<int>(lines_.size()); ++i) {
      const float y = content_y + static_cast<float>(i) * lh;
      if (y + lh < bounds_.y || y > bounds_.y + bounds_.h) {
        continue;
      }
      const std::wstring& line = lines_[static_cast<size_t>(i)];
      const size_t line_begin = line_starts_[static_cast<size_t>(i)];
      const size_t line_end = line_begin + line.size();

      if (focused() && HasSelection() && sel_a < sel_b) {
        const size_t a = (std::max)(sel_a, line_begin);
        const size_t b = (std::min)(sel_b, line_end);
        if (a < b) {
          const float x0 = content_x + mx::MeasureUiTextWidth(
                                           line.substr(0, a - line_begin),
                                           fs, font);
          const float x1 = content_x + mx::MeasureUiTextWidth(
                                           line.substr(0, b - line_begin),
                                           fs, font);
          canvas.FillRect(RectF{x0, y, std::max(1.f, x1 - x0), lh},
                          th.selection);
        }
      }

      if (!line.empty()) {
        canvas.DrawText(line, RectF{content_x, y, content_w, lh}, th.text, fs,
                        font, mx::TextHAlign::Left);
      }
    }

    if (focused() && !HasSelection()) {
      int line = 0;
      int col = 0;
      IndexToLineCol(caret_, &line, &col);
      const std::wstring& s = lines_[static_cast<size_t>(line)];
      float caret_x =
          content_x + mx::MeasureUiTextWidth(
                          s.substr(0, static_cast<size_t>(col)), fs, font);
      if (!composition_.empty()) {
        caret_x += mx::MeasureUiTextWidth(composition_, fs, font);
      }
      const float y = content_y + static_cast<float>(line) * lh;
      canvas.FillRect(RectF{caret_x, y, 1.5f, lh}, th.text);
    }
  }

  canvas.PopAxisAlignedClip();
}

void TextArea::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  composition_.clear();

  const DWORD now = GetTickCount();
  if (pending_triple_click_ &&
      (now - last_double_click_ms_) <= GetDoubleClickTime()) {
    pending_triple_click_ = false;
    SelectLineAt(HitTestCaret(e.x, e.y));
    selecting_ = false;
    Invalidate();
    return;
  }
  pending_triple_click_ = false;

  selecting_ = true;
  SetCaret(HitTestCaret(e.x, e.y), false);
  Invalidate();
}

void TextArea::OnMouseMove(const MouseEvent& e) {
  if (!selecting_) {
    return;
  }
  SetCaret(HitTestCaret(e.x, e.y), true);
  Invalidate();
}

void TextArea::OnMouseUp(const MouseEvent& e) {
  if (e.button == MouseButton::Left) {
    selecting_ = false;
  }
}

void TextArea::OnMouseDoubleClick(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  composition_.clear();
  selecting_ = false;
  SelectWordAt(HitTestCaret(e.x, e.y));
  pending_triple_click_ = true;
  last_double_click_ms_ = GetTickCount();
  Invalidate();
}

void TextArea::OnMouseWheel(const MouseEvent& e) {
  const float lh = LineHeight();
  scroll_y_ -= (e.wheel_delta / 120.f) * lh * 3.f;
  const float content_h = lh * static_cast<float>(lines_.size());
  const float view_h = std::max(0.f, bounds_.h - kPad * 2.f);
  scroll_y_ = std::clamp(scroll_y_, 0.f, std::max(0.f, content_h - view_h));
}

void TextArea::OnKey(const KeyEvent& e) {
  if (!e.down) {
    return;
  }
  if (e.ctrl) {
    HandleShortcut(e.vk);
    return;
  }
  int line = 0;
  int col = 0;
  IndexToLineCol(caret_, &line, &col);
  const bool extend = e.shift;
  switch (e.vk) {
    case VK_LEFT:
      if (caret_ > 0) {
        SetCaret(caret_ - 1, extend);
      }
      break;
    case VK_RIGHT:
      if (caret_ < text_.size()) {
        SetCaret(caret_ + 1, extend);
      }
      break;
    case VK_UP:
      SetCaret(LineColToIndex(line - 1, col), extend);
      break;
    case VK_DOWN:
      SetCaret(LineColToIndex(line + 1, col), extend);
      break;
    case VK_HOME:
      SetCaret(LineColToIndex(line, 0), extend);
      break;
    case VK_END:
      SetCaret(LineColToIndex(line, static_cast<int>(
                                        lines_[static_cast<size_t>(line)].size())),
               extend);
      break;
    case VK_BACK:
      if (HasSelection()) {
        DeleteSelection();
      } else if (caret_ > 0) {
        text_.erase(caret_ - 1, 1);
        --caret_;
        sel_start_ = caret_;
        RebuildLines();
        EnsureCaretVisible();
        NotifyChanged();
      }
      break;
    case VK_DELETE:
      if (HasSelection()) {
        DeleteSelection();
      } else if (caret_ < text_.size()) {
        text_.erase(caret_, 1);
        sel_start_ = caret_;
        RebuildLines();
        NotifyChanged();
      }
      break;
    case VK_RETURN:
      InsertText(L"\n");
      break;
    default:
      break;
  }
}

void TextArea::OnChar(wchar_t ch) {
  if (ch < 32 && ch != L'\t') {
    return;
  }
  if (ch == L'\t') {
    InsertText(L"  ");
    return;
  }
  InsertText(std::wstring(1, ch));
}

void TextArea::OnFocus() {}
void TextArea::OnBlur() {
  composition_.clear();
  selecting_ = false;
}

bool TextArea::WantsIme() const { return focused(); }

void TextArea::OnImeComposition(const std::wstring& composition) {
  composition_ = composition;
}

void TextArea::OnImeResult(const std::wstring& result) {
  composition_.clear();
  InsertText(result);
}

void TextArea::OnImeEnd() { composition_.clear(); }

AccRole TextArea::acc_role() const {
  return AccRole::Edit;
}

std::wstring TextArea::AccDefaultName() const {
  return placeholder_;
}

std::wstring TextArea::AccValue() const {
  return text_;
}

bool TextArea::AccSetValue(const std::wstring& value) {
  set_text(value);
  return true;
}

}  // namespace mx::ui
