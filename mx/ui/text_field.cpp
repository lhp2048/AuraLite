#include "mx/ui/text_field.h"

#include "mx/ui/theme.h"
#include "mx/ui/window.h"

#include <dwrite.h>

#include <algorithm>
#include <cstring>
#include <cwctype>

namespace mx::ui {
namespace {

IDWriteFactory* SharedDWrite() {
  static IDWriteFactory* factory = nullptr;
  if (!factory) {
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                        reinterpret_cast<IUnknown**>(&factory));
  }
  return factory;
}

float MeasureWidth(const std::wstring& text, float font_size) {
  if (text.empty()) {
    return 0.f;
  }
  IDWriteFactory* factory = SharedDWrite();
  if (!factory) {
    return font_size * 0.55f * static_cast<float>(text.size());
  }
  IDWriteTextFormat* format = nullptr;
  HRESULT hr = factory->CreateTextFormat(
      Theme::Active().font_ui.c_str(), nullptr, DWRITE_FONT_WEIGHT_NORMAL,
      DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, font_size, L"zh-cn",
      &format);
  if (FAILED(hr) || !format) {
    return font_size * 0.55f * static_cast<float>(text.size());
  }
  IDWriteTextLayout* layout = nullptr;
  hr = factory->CreateTextLayout(text.c_str(),
                                 static_cast<UINT32>(text.size()), format,
                                 100000.f, font_size * 2.f, &layout);
  format->Release();
  if (FAILED(hr) || !layout) {
    return font_size * 0.55f * static_cast<float>(text.size());
  }
  DWRITE_TEXT_METRICS metrics = {};
  hr = layout->GetMetrics(&metrics);
  layout->Release();
  if (FAILED(hr)) {
    return font_size * 0.55f * static_cast<float>(text.size());
  }
  return metrics.widthIncludingTrailingWhitespace;
}

bool IsWordChar(wchar_t c) {
  if (c == L'_') {
    return true;
  }
  if (iswalnum(static_cast<wint_t>(c))) {
    return true;
  }
  // CJK / kana / hangul — treat as word characters for double-click.
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

TextField::TextField() {
  set_focusable(true);
  fill_width();
  fixed_height(36.f);
  set_preferred_width(280.f);
}

TextField::~TextField() {
  SyncCaretAnim(false);
}

void TextField::ResetCaretBlink() {
  caret_blink_start_ = GetTickCount();
}

void TextField::SyncCaretAnim(bool on) {
  if (on == caret_anim_registered_) {
    return;
  }
  Window* w = host_window();
  if (!w) {
    caret_anim_registered_ = false;
    return;
  }
  if (on) {
    w->RegisterAnimation();
    caret_anim_registered_ = true;
  } else {
    w->UnregisterAnimation();
    caret_anim_registered_ = false;
  }
}

bool TextField::CaretVisible() const {
  if (!focused() || HasSelection()) {
    return false;
  }
  const DWORD elapsed = GetTickCount() - caret_blink_start_;
  return ((elapsed / kCaretBlinkMs) % 2) == 0;
}

TextField& TextField::text(const std::wstring& t) {
  set_text(t);
  return *this;
}

TextField& TextField::placeholder(const std::wstring& t) {
  placeholder_ = t;
  return *this;
}

TextField& TextField::password(bool enabled) {
  password_ = enabled;
  return *this;
}

TextField& TextField::font_size(float size) {
  font_size_ = size;
  return *this;
}

TextField& TextField::preferred_size(float w, float h) {
  fixed_width(w);
  fixed_height(h);
  return *this;
}

TextField& TextField::on_change(ChangeHandler handler) {
  on_change_ = std::move(handler);
  return *this;
}

TextField& TextField::on_submit(std::function<void()> handler) {
  on_submit_ = std::move(handler);
  return *this;
}

void TextField::set_text(const std::wstring& t) {
  if (text_ == t) {
    return;
  }
  text_ = t;
  caret_ = text_.size();
  sel_start_ = caret_;
  composition_.clear();
  NotifyChanged();
}

void TextField::SelectAll() {
  sel_start_ = 0;
  caret_ = text_.size();
  ResetCaretBlink();
}

void TextField::SelectWordAt(size_t pos) {
  size_t a = 0;
  size_t b = 0;
  ExpandWordRange(text_, pos, &a, &b);
  if (a == b) {
    SetCursor(a, false);
    return;
  }
  sel_start_ = a;
  caret_ = b;
  ResetCaretBlink();
}

void TextField::ClearSelection() {
  sel_start_ = caret_;
}

bool TextField::HasSelection() const {
  return sel_start_ != caret_;
}

SizeF TextField::Measure(float max_w, float max_h) {
  const float hug_w = preferred_width() > 0.f ? preferred_width() : 280.f;
  const float hug_h = preferred_height() > 0.f ? preferred_height() : 36.f;
  return ResolveSize(max_w, max_h, hug_w, hug_h);
}

std::wstring TextField::DisplayText() const {
  if (password_) {
    return std::wstring(text_.size(), kPasswordChar);
  }
  return text_;
}

std::wstring TextField::VisibleText() const {
  std::wstring shown = DisplayText();
  if (!composition_.empty() && !password_) {
    shown.insert(caret_, composition_);
  }
  return shown;
}

void TextField::GetOrderedSelection(size_t* start, size_t* end) const {
  if (!start || !end) {
    return;
  }
  *start = (std::min)(sel_start_, caret_);
  *end = (std::max)(sel_start_, caret_);
}

float TextField::TextWidth(const std::wstring& s) const {
  return MeasureWidth(s, ResolveFontSize(font_size_));
}

void TextField::SetCursor(size_t pos, bool extend_selection) {
  if (pos > text_.size()) {
    pos = text_.size();
  }
  caret_ = pos;
  if (!extend_selection) {
    sel_start_ = caret_;
  }
  ResetCaretBlink();
}

size_t TextField::HitTestCursor(float window_x) const {
  const std::wstring shown = DisplayText();
  const float x = window_x - bounds_.x - kPadX;
  if (x <= 0.f || shown.empty()) {
    return 0;
  }
  for (size_t pos = 0; pos < shown.size(); ++pos) {
    const float left = TextWidth(shown.substr(0, pos));
    const float right = TextWidth(shown.substr(0, pos + 1));
    if (x < (left + right) * 0.5f) {
      return pos;
    }
  }
  return shown.size();
}

void TextField::DeleteSelection() {
  if (!HasSelection()) {
    return;
  }
  size_t a = 0;
  size_t b = 0;
  GetOrderedSelection(&a, &b);
  text_.erase(a, b - a);
  caret_ = a;
  sel_start_ = a;
  NotifyChanged();
}

void TextField::InsertText(const std::wstring& text) {
  if (text.empty()) {
    return;
  }
  if (HasSelection()) {
    size_t a = 0;
    size_t b = 0;
    GetOrderedSelection(&a, &b);
    text_.erase(a, b - a);
    caret_ = a;
  }
  text_.insert(caret_, text);
  caret_ += text.size();
  sel_start_ = caret_;
  ResetCaretBlink();
  NotifyChanged();
}

void TextField::DeleteSelectionOrChar(bool forward) {
  if (HasSelection()) {
    DeleteSelection();
    ResetCaretBlink();
    return;
  }
  if (text_.empty()) {
    return;
  }
  if (forward) {
    if (caret_ < text_.size()) {
      text_.erase(caret_, 1);
      sel_start_ = caret_;
      ResetCaretBlink();
      NotifyChanged();
    }
  } else if (caret_ > 0) {
    --caret_;
    text_.erase(caret_, 1);
    sel_start_ = caret_;
    ResetCaretBlink();
    NotifyChanged();
  }
}

void TextField::NotifyChanged() {
  NotifyAccValueChanged();
  if (on_change_) {
    on_change_(text_);
  }
}

bool TextField::CopyToClipboard() const {
  if (!HasSelection() || password_) {
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

bool TextField::PasteFromClipboard() {
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
        if (ch != L'\r' && ch != L'\n' && ch != L'\t') {
          filtered.push_back(ch);
        }
      }
      InsertText(filtered);
      ok = true;
    }
  }
  CloseClipboard();
  return ok;
}

bool TextField::CutToClipboard() {
  if (password_) {
    return false;
  }
  if (!CopyToClipboard()) {
    return false;
  }
  DeleteSelection();
  return true;
}

void TextField::HandleShortcut(UINT vk) {
  switch (vk) {
    case 'A':
      SelectAll();
      break;
    case 'C':
      CopyToClipboard();
      break;
    case 'X':
      CutToClipboard();
      break;
    case 'V':
      PasteFromClipboard();
      break;
    default:
      break;
  }
}

void TextField::Paint(mx::Canvas& canvas) {
  if (!visible()) {
    return;
  }
  const ThemeTokens& th = Theme::Active();
  const float fs = ResolveFontSize(font_size_);
  canvas.FillRoundedRect(bounds_, 6.f, 6.f, th.surface);
  canvas.DrawRect(bounds_, focused() ? th.border_focus : th.border, 1.5f);

  const RectF text_rect{bounds_.x + kPadX, bounds_.y,
                        (std::max)(0.f, bounds_.w - kPadX * 2.f), bounds_.h};

  const std::wstring shown = VisibleText();
  const bool show_placeholder =
      text_.empty() && composition_.empty() && !placeholder_.empty();

  if (focused() && HasSelection() && !show_placeholder) {
    size_t a = 0;
    size_t b = 0;
    GetOrderedSelection(&a, &b);
    const std::wstring base = DisplayText();
    if (a <= base.size() && b <= base.size() && a < b) {
      const float x0 = text_rect.x + TextWidth(base.substr(0, a));
      const float x1 = text_rect.x + TextWidth(base.substr(0, b));
      const float line_h = fs + 4.f;
      const float ty = bounds_.y + (bounds_.h - line_h) * 0.5f;
      canvas.FillRect(RectF{x0, ty, (std::max)(1.f, x1 - x0), line_h},
                      th.selection);
    }
  }

  if (show_placeholder) {
    canvas.DrawText(placeholder_, text_rect, th.text_muted, fs,
                    th.font_ui.c_str(), mx::TextHAlign::Left);
  } else if (!shown.empty()) {
    canvas.DrawText(shown, text_rect, th.text, fs, th.font_ui.c_str(),
                    mx::TextHAlign::Left);
  }

  if (focused() && !composition_.empty() && !password_) {
    const std::wstring before = DisplayText().substr(0, caret_);
    const float x0 = text_rect.x + TextWidth(before);
    const float cw = TextWidth(composition_);
    const float uy = bounds_.y + bounds_.h * 0.5f + fs * 0.45f;
    canvas.FillRect(RectF{x0, uy, (std::max)(1.f, cw), 2.f}, th.accent);
  }

  if (CaretVisible()) {
    const std::wstring before = DisplayText().substr(0, caret_);
    float caret_x = text_rect.x + TextWidth(before);
    if (!composition_.empty() && !password_) {
      caret_x += TextWidth(composition_);
    }
    const float line_h = fs + 4.f;
    const float ty = bounds_.y + (bounds_.h - line_h) * 0.5f;
    canvas.FillRect(RectF{caret_x, ty, 1.5f, line_h}, th.text);
  }
}

void TextField::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  composition_.clear();

  // Triple-click: second click arrives as DBLCLK; third is DOWN within the
  // system double-click interval after that.
  const DWORD now = GetTickCount();
  if (pending_triple_click_ &&
      (now - last_double_click_ms_) <= GetDoubleClickTime()) {
    pending_triple_click_ = false;
    SelectAll();
    selecting_ = false;
    Invalidate();
    return;
  }
  pending_triple_click_ = false;

  selecting_ = true;
  SetCursor(HitTestCursor(e.x), false);
  Invalidate();
}

void TextField::OnMouseMove(const MouseEvent& e) {
  if (!selecting_) {
    return;
  }
  SetCursor(HitTestCursor(e.x), true);
  Invalidate();
}

void TextField::OnMouseUp(const MouseEvent&) {
  selecting_ = false;
}

void TextField::OnMouseDoubleClick(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  composition_.clear();
  selecting_ = false;
  if (password_) {
    SelectAll();
  } else {
    SelectWordAt(HitTestCursor(e.x));
  }
  pending_triple_click_ = true;
  last_double_click_ms_ = GetTickCount();
  Invalidate();
}

void TextField::OnKey(const KeyEvent& e) {
  if (!e.down) {
    return;
  }

  if (e.ctrl && !e.alt) {
    HandleShortcut(e.vk);
    return;
  }

  switch (e.vk) {
    case VK_RETURN:
      if (on_submit_) {
        on_submit_();
      }
      break;
    case VK_BACK:
      if (!composition_.empty()) {
        composition_.clear();
      } else {
        DeleteSelectionOrChar(false);
      }
      break;
    case VK_DELETE:
      if (composition_.empty()) {
        DeleteSelectionOrChar(true);
      }
      break;
    case VK_LEFT:
      if (HasSelection() && !e.shift) {
        size_t a = 0;
        size_t b = 0;
        GetOrderedSelection(&a, &b);
        SetCursor(a, false);
      } else if (caret_ > 0) {
        SetCursor(caret_ - 1, e.shift);
      } else if (!e.shift) {
        ClearSelection();
      }
      break;
    case VK_RIGHT:
      if (HasSelection() && !e.shift) {
        size_t a = 0;
        size_t b = 0;
        GetOrderedSelection(&a, &b);
        SetCursor(b, false);
      } else if (caret_ < text_.size()) {
        SetCursor(caret_ + 1, e.shift);
      } else if (!e.shift) {
        ClearSelection();
      }
      break;
    case VK_HOME:
      SetCursor(0, e.shift);
      break;
    case VK_END:
      SetCursor(text_.size(), e.shift);
      break;
    default:
      break;
  }
}

void TextField::OnChar(wchar_t ch) {
  if (ch < 32 || ch == 127) {
    return;
  }
  composition_.clear();
  InsertText(std::wstring(1, ch));
}

void TextField::OnFocus() {
  ResetCaretBlink();
  SyncCaretAnim(true);
}

void TextField::OnBlur() {
  SyncCaretAnim(false);
  composition_.clear();
  selecting_ = false;
}

bool TextField::WantsIme() const {
  // Password fields still need an IME association. Detaching IME (old behavior)
  // breaks WM_CHAR under Chinese IMEs — caret shows but typing does nothing.
  return focused();
}

void TextField::OnImeComposition(const std::wstring& composition) {
  // Do not mirror composition plaintext into a password field.
  if (password_) {
    composition_.clear();
    return;
  }
  composition_ = composition;
}

void TextField::OnImeResult(const std::wstring& result) {
  if (result.empty()) {
    return;
  }
  composition_.clear();
  InsertText(result);
}

void TextField::OnImeEnd() {
  composition_.clear();
}

AccRole TextField::acc_role() const {
  return AccRole::Edit;
}

std::wstring TextField::AccDefaultName() const {
  return placeholder_;
}

AccState TextField::acc_state() const {
  AccState s = Node::acc_state();
  s.password = password_;
  return s;
}

std::wstring TextField::AccValue() const {
  return password_ ? std::wstring{} : text_;
}

bool TextField::AccSetValue(const std::wstring& value) {
  set_text(value);
  return true;
}

}  // namespace mx::ui
