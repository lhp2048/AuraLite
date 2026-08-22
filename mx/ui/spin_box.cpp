#include "mx/ui/spin_box.h"

#include "mx/ui/theme.h"
#include "mx/ui/window.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace mx::ui {
namespace {

double ClampOrWrap(double v, double lo, double hi, bool wrap) {
  if (hi < lo) {
    std::swap(lo, hi);
  }
  if (wrap && hi > lo) {
    const double span = hi - lo;
    if (v > hi) {
      v = lo + std::fmod(v - lo, span);
    } else if (v < lo) {
      v = hi - std::fmod(lo - v, span);
    }
  }
  return std::clamp(v, lo, hi);
}

}  // namespace

SpinBox::SpinBox() {
  set_focusable(true);
  fill_width();
  fixed_height(36.f);
}

SpinBox& SpinBox::value(double v) {
  SetClamped(v, false);
  return *this;
}

SpinBox& SpinBox::min_value(double v) {
  min_ = v;
  SetClamped(value_, false);
  return *this;
}

SpinBox& SpinBox::max_value(double v) {
  max_ = v;
  SetClamped(value_, false);
  return *this;
}

SpinBox& SpinBox::step(double s) {
  step_ = s > 0.0 ? s : 1.0;
  return *this;
}

SpinBox& SpinBox::decimals(int n) {
  decimals_ = std::clamp(n, 0, 6);
  return *this;
}

SpinBox& SpinBox::wrap(bool enable) {
  wrap_ = enable;
  return *this;
}

SpinBox& SpinBox::font_size(float size) {
  font_size_ = size;
  return *this;
}

SpinBox& SpinBox::on_changed(ChangeHandler handler) {
  on_changed_ = std::move(handler);
  return *this;
}

void SpinBox::SetClamped(double v, bool notify) {
  if (editing_) {
    editing_ = false;
    edit_buffer_.clear();
  }
  const double next = ClampOrWrap(v, min_, max_, wrap_);
  if (next == value_) {
    Invalidate();
    return;
  }
  value_ = next;
  NotifyAccRangeChanged();
  NotifyAccValueChanged();
  if (notify && on_changed_) {
    on_changed_(value_);
  }
  Invalidate();
}

void SpinBox::Nudge(int dir, bool large) {
  ClearEditBuffer();
  const double s = step_ > 0.0 ? step_ : 1.0;
  const double delta = (large ? s * 10.0 : s) * static_cast<double>(dir);
  SetClamped(value_ + delta, true);
}

std::wstring SpinBox::FormatValue() const {
  if (editing_) {
    return edit_buffer_.empty() ? L" " : edit_buffer_;
  }
  wchar_t buf[64] = {};
  if (decimals_ <= 0) {
    swprintf_s(buf, L"%.0f", value_);
  } else {
    swprintf_s(buf, L"%.*f", decimals_, value_);
  }
  return buf;
}

void SpinBox::ClearEditBuffer() {
  if (!editing_ && edit_buffer_.empty()) {
    return;
  }
  editing_ = false;
  edit_buffer_.clear();
  Invalidate();
}

void SpinBox::CommitEditBuffer(bool notify) {
  if (!editing_) {
    return;
  }
  double v = 0.0;
  if (!edit_buffer_.empty() && swscanf_s(edit_buffer_.c_str(), L"%lf", &v) == 1) {
    editing_ = false;
    edit_buffer_.clear();
    SetClamped(v, notify);
    return;
  }
  ClearEditBuffer();
}

RectF SpinBox::ChevronRect() const {
  return RectF{bounds_.x + bounds_.w - kChevronW, bounds_.y, kChevronW,
               bounds_.h};
}

int SpinBox::HitChevron(float x, float y) const {
  const RectF c = ChevronRect();
  if (!ContainsPoint(c, x, y)) {
    return 0;
  }
  return (y < c.y + c.h * 0.5f) ? 1 : -1;
}

AccRole SpinBox::acc_role() const {
  if (acc_role_override_) {
    return *acc_role_override_;
  }
  return AccRole::Spinner;
}

std::wstring SpinBox::AccDefaultName() const {
  return {};
}

std::wstring SpinBox::AccValue() const {
  return FormatValue();
}

bool SpinBox::AccSetValue(const std::wstring& value) {
  double v = 0.0;
  if (swscanf_s(value.c_str(), L"%lf", &v) != 1) {
    return false;
  }
  SetClamped(v, true);
  return true;
}

double SpinBox::AccRangeValue() const {
  return value_;
}

double SpinBox::AccRangeMinimum() const {
  return min_;
}

double SpinBox::AccRangeMaximum() const {
  return max_;
}

double SpinBox::AccRangeSmallChange() const {
  return step_ > 0.0 ? step_ : 1.0;
}

double SpinBox::AccRangeLargeChange() const {
  return AccRangeSmallChange() * 10.0;
}

bool SpinBox::AccRangeReadOnly() const {
  return false;
}

bool SpinBox::AccSetRangeValue(double value) {
  SetClamped(value, true);
  return true;
}

SizeF SpinBox::Measure(float max_w, float max_h) {
  return ResolveSize(max_w, max_h,
                     preferred_width() > 0.f ? preferred_width() : max_w, 36.f);
}

void SpinBox::Paint(mx::Canvas& canvas) {
  if (!visible()) {
    return;
  }
  const ThemeTokens& th = Theme::Active();
  canvas.FillRoundedRect(bounds_, 6.f, 6.f, th.surface);
  canvas.DrawRect(bounds_, focused() ? th.border_focus : th.border,
                  focused() ? 1.5f : 1.f);

  const RectF chev = ChevronRect();
  canvas.FillRect(RectF{chev.x, bounds_.y + 1.f, 1.f, bounds_.h - 2.f},
                  th.divider);
  if (hover_dir_ == 1) {
    canvas.FillRect(RectF{chev.x + 1.f, chev.y, chev.w - 1.f, chev.h * 0.5f},
                    th.accent_soft);
  } else if (hover_dir_ == -1) {
    canvas.FillRect(
        RectF{chev.x + 1.f, chev.y + chev.h * 0.5f, chev.w - 1.f, chev.h * 0.5f},
        th.accent_soft);
  }

  const float cx = chev.x + chev.w * 0.5f;
  const float up_y = chev.y + chev.h * 0.25f;
  const float dn_y = chev.y + chev.h * 0.75f;
  canvas.DrawLine(cx - 4.f, up_y + 2.f, cx, up_y - 2.f, th.glyph, 1.5f);
  canvas.DrawLine(cx, up_y - 2.f, cx + 4.f, up_y + 2.f, th.glyph, 1.5f);
  canvas.DrawLine(cx - 4.f, dn_y - 2.f, cx, dn_y + 2.f, th.glyph, 1.5f);
  canvas.DrawLine(cx, dn_y + 2.f, cx + 4.f, dn_y - 2.f, th.glyph, 1.5f);

  const RectF text{bounds_.x + 10.f, bounds_.y,
                   std::max(0.f, bounds_.w - kChevronW - 14.f), bounds_.h};
  canvas.DrawText(FormatValue(), text, th.text, ResolveFontSize(font_size_),
                  th.font_ui.c_str(), mx::TextHAlign::Left);
}

void SpinBox::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  const int dir = HitChevron(e.x, e.y);
  if (dir != 0) {
    Nudge(dir, false);
  } else {
    // Click value area: start replace-style edit.
    editing_ = true;
    edit_buffer_.clear();
    Invalidate();
  }
}

void SpinBox::OnMouseMove(const MouseEvent& e) {
  const int dir = HitChevron(e.x, e.y);
  if (dir != hover_dir_) {
    hover_dir_ = dir;
    Invalidate();
  }
}

void SpinBox::OnMouseLeave(const MouseEvent&) {
  if (hover_dir_ != 0) {
    hover_dir_ = 0;
    Invalidate();
  }
}

void SpinBox::OnFocus() {}

void SpinBox::OnBlur() {
  CommitEditBuffer(true);
}

void SpinBox::OnKey(const KeyEvent& e) {
  if (!e.down) {
    return;
  }
  if (e.vk == VK_UP || e.vk == VK_RIGHT) {
    Nudge(1, false);
  } else if (e.vk == VK_DOWN || e.vk == VK_LEFT) {
    Nudge(-1, false);
  } else if (e.vk == VK_PRIOR) {
    Nudge(1, true);
  } else if (e.vk == VK_NEXT) {
    Nudge(-1, true);
  } else if (e.vk == VK_HOME) {
    ClearEditBuffer();
    SetClamped(min_, true);
  } else if (e.vk == VK_END) {
    ClearEditBuffer();
    SetClamped(max_, true);
  } else if (e.vk == VK_RETURN) {
    CommitEditBuffer(true);
  } else if (e.vk == VK_ESCAPE) {
    ClearEditBuffer();
  } else if (e.vk == VK_BACK) {
    if (!editing_) {
      const std::wstring cur = FormatValue();
      editing_ = true;
      edit_buffer_ = cur;
    }
    if (!edit_buffer_.empty()) {
      edit_buffer_.pop_back();
      Invalidate();
    }
  } else if ((e.vk >= '0' && e.vk <= '9') ||
             (e.vk >= VK_NUMPAD0 && e.vk <= VK_NUMPAD9) ||
             e.vk == VK_OEM_PERIOD || e.vk == VK_DECIMAL) {
    wchar_t ch = 0;
    if (e.vk >= '0' && e.vk <= '9') {
      ch = static_cast<wchar_t>(e.vk);
    } else if (e.vk >= VK_NUMPAD0 && e.vk <= VK_NUMPAD9) {
      ch = static_cast<wchar_t>(L'0' + (e.vk - VK_NUMPAD0));
    } else if (decimals_ > 0) {
      ch = L'.';
    }
    if (ch == 0) {
      return;
    }
    if (!editing_) {
      editing_ = true;
      edit_buffer_.clear();
    }
    if (ch == L'.') {
      if (edit_buffer_.find(L'.') != std::wstring::npos) {
        return;
      }
    }
    if (edit_buffer_.size() >= 12) {
      return;
    }
    edit_buffer_.push_back(ch);
    Invalidate();
  }
}

void SpinBox::OnMouseWheel(const MouseEvent& e) {
  if (e.wheel_delta > 0) {
    Nudge(1, false);
  } else if (e.wheel_delta < 0) {
    Nudge(-1, false);
  }
}

}  // namespace mx::ui
