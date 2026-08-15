#include "auralite/ui/text_field.h"

#include "auralite/ui/theme.h"

#include <dwrite.h>

#include <algorithm>
#include <cstring>

namespace auralite::ui {
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

}  // namespace

TextField::TextField() {
  set_focusable(true);
  fill_width();
  fixed_height(36.f);
  set_preferred_width(280.f);
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
  NotifyChanged();
}

void TextField::DeleteSelectionOrChar(bool forward) {
  if (HasSelection()) {
    DeleteSelection();
    return;
  }
  if (text_.empty()) {
    return;
  }
  if (forward) {
    if (caret_ < text_.size()) {
      text_.erase(caret_, 1);
      sel_start_ = caret_;
      NotifyChanged();
    }
  } else if (caret_ > 0) {
    --caret_;
    text_.erase(caret_, 1);
    sel_start_ = caret_;
    NotifyChanged();
  }
}

void TextField::NotifyChanged() {
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

void TextField::Paint(auralite::Canvas& canvas) {
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
                    th.font_ui.c_str(), auralite::TextHAlign::Left);
  } else if (!shown.empty()) {
    canvas.DrawText(shown, text_rect, th.text, fs, th.font_ui.c_str(),
                    auralite::TextHAlign::Left);
  }

  if (focused() && !composition_.empty() && !password_) {
    const std::wstring before = DisplayText().substr(0, caret_);
    const float x0 = text_rect.x + TextWidth(before);
    const float cw = TextWidth(composition_);
    const float uy = bounds_.y + bounds_.h * 0.5f + fs * 0.45f;
    canvas.FillRect(RectF{x0, uy, (std::max)(1.f, cw), 2.f}, th.accent);
  }

  if (focused() && !HasSelection()) {
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
  selecting_ = true;
  SetCursor(HitTestCursor(e.x), false);
}

void TextField::OnMouseMove(const MouseEvent& e) {
  if (!selecting_) {
    return;
  }
  SetCursor(HitTestCursor(e.x), true);
}

void TextField::OnMouseUp(const MouseEvent&) {
  selecting_ = false;
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

void TextField::OnFocus() {}

void TextField::OnBlur() {
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

}  // namespace auralite::ui
