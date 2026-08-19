#include "auralite/ui/color_picker.h"

#include "auralite/ui/theme.h"
#include "auralite/ui/theme_yaml.h"
#include "auralite/ui/window.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace auralite::ui {
namespace {

constexpr float kPad = 10.f;
constexpr float kSwatch = 22.f;
constexpr float kGap = 6.f;
constexpr float kSimpleCols = 8.f;
constexpr float kSimpleRows = 3.f;
constexpr float kSvW = 220.f;
constexpr float kSvH = 150.f;
constexpr float kBarH = 14.f;
constexpr float kPreview = 36.f;

struct Hsv {
  float h = 0.f;  // 0..360
  float s = 0.f;  // 0..1
  float v = 0.f;  // 0..1
};

float Clamp01(float v) { return std::clamp(v, 0.f, 1.f); }

uint8_t ToByte(float c) {
  return static_cast<uint8_t>(
      std::lround(Clamp01(c) * 255.f));
}

ColorF ClampColor(ColorF c) {
  c.r = Clamp01(c.r);
  c.g = Clamp01(c.g);
  c.b = Clamp01(c.b);
  c.a = Clamp01(c.a);
  return c;
}

Hsv RgbToHsv(const ColorF& c) {
  const float r = Clamp01(c.r);
  const float g = Clamp01(c.g);
  const float b = Clamp01(c.b);
  const float mx = (std::max)({r, g, b});
  const float mn = (std::min)({r, g, b});
  const float d = mx - mn;
  Hsv out;
  out.v = mx;
  out.s = (mx <= 0.f) ? 0.f : d / mx;
  if (d <= 1e-6f) {
    out.h = 0.f;
  } else if (mx == r) {
    out.h = 60.f * std::fmod((g - b) / d, 6.f);
  } else if (mx == g) {
    out.h = 60.f * ((b - r) / d + 2.f);
  } else {
    out.h = 60.f * ((r - g) / d + 4.f);
  }
  if (out.h < 0.f) {
    out.h += 360.f;
  }
  return out;
}

ColorF HsvToRgb(float h, float s, float v, float a) {
  h = std::fmod(h, 360.f);
  if (h < 0.f) {
    h += 360.f;
  }
  s = Clamp01(s);
  v = Clamp01(v);
  const float c = v * s;
  const float x = c * (1.f - std::fabs(std::fmod(h / 60.f, 2.f) - 1.f));
  const float m = v - c;
  float r = 0.f;
  float g = 0.f;
  float b = 0.f;
  if (h < 60.f) {
    r = c;
    g = x;
  } else if (h < 120.f) {
    r = x;
    g = c;
  } else if (h < 180.f) {
    g = c;
    b = x;
  } else if (h < 240.f) {
    g = x;
    b = c;
  } else if (h < 300.f) {
    r = x;
    b = c;
  } else {
    r = c;
    b = x;
  }
  return ColorF(r + m, g + m, b + m, Clamp01(a));
}

ColorF HueColor(float h) { return HsvToRgb(h, 1.f, 1.f, 1.f); }

std::wstring FormatHex(const ColorF& c, bool with_alpha) {
  wchar_t buf[16] = {};
  if (with_alpha) {
    swprintf_s(buf, L"#%02X%02X%02X%02X", ToByte(c.r), ToByte(c.g), ToByte(c.b),
               ToByte(c.a));
  } else {
    swprintf_s(buf, L"#%02X%02X%02X", ToByte(c.r), ToByte(c.g), ToByte(c.b));
  }
  return buf;
}

std::string NarrowHex(const std::wstring& wide) {
  std::string out;
  out.reserve(wide.size());
  for (wchar_t ch : wide) {
    if (ch >= 32 && ch < 127) {
      out.push_back(static_cast<char>(ch));
    }
  }
  return out;
}

bool ParseHexWide(const std::wstring& s, ColorF* out) {
  return out && ParseColorHex(NarrowHex(s), out);
}

bool ColorsEqual(const ColorF& a, const ColorF& b) {
  return ToByte(a.r) == ToByte(b.r) && ToByte(a.g) == ToByte(b.g) &&
         ToByte(a.b) == ToByte(b.b) && ToByte(a.a) == ToByte(b.a);
}

const ColorF kPresets[] = {
    ColorF::FromRgb(0, 0, 0),       ColorF::FromRgb(64, 64, 64),
    ColorF::FromRgb(128, 128, 128), ColorF::FromRgb(192, 192, 192),
    ColorF::FromRgb(255, 255, 255), ColorF::FromRgb(180, 50, 50),
    ColorF::FromRgb(220, 100, 40),  ColorF::FromRgb(210, 160, 60),
    ColorF::FromRgb(60, 160, 80),   ColorF::FromRgb(40, 110, 200),
    ColorF::FromRgb(76, 139, 245),  ColorF::FromRgb(120, 80, 200),
    ColorF::FromRgb(200, 80, 160),  ColorF::FromRgb(20, 160, 180),
    ColorF::FromRgb(90, 60, 40),    ColorF::FromRgb(230, 238, 250),
    ColorF::FromRgb(255, 230, 230), ColorF::FromRgb(230, 255, 235),
    ColorF::FromRgb(255, 245, 210), ColorF::FromRgb(235, 230, 255),
    ColorF::FromRgb(40, 40, 40),    ColorF::FromRgb(255, 80, 80),
    ColorF::FromRgb(80, 200, 120),  ColorF::FromRgb(80, 160, 255),
};
constexpr int kPresetCount = static_cast<int>(sizeof(kPresets) / sizeof(kPresets[0]));

void FillChecker(auralite::Canvas& canvas, const RectF& r, float cell = 6.f) {
  canvas.FillRect(r, ColorF::FromRgb(220, 220, 220));
  const int cols = (std::max)(1, static_cast<int>(std::ceil(r.w / cell)));
  const int rows = (std::max)(1, static_cast<int>(std::ceil(r.h / cell)));
  for (int y = 0; y < rows; ++y) {
    for (int x = 0; x < cols; ++x) {
      if (((x + y) & 1) == 0) {
        continue;
      }
      const float cx = r.x + x * cell;
      const float cy = r.y + y * cell;
      const float cw = (std::min)(cell, r.x + r.w - cx);
      const float ch = (std::min)(cell, r.y + r.h - cy);
      if (cw > 0.f && ch > 0.f) {
        canvas.FillRect(RectF{cx, cy, cw, ch}, ColorF::FromRgb(255, 255, 255));
      }
    }
  }
}

bool ContainsPoint(const RectF& r, float x, float y) {
  return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

}  // namespace

class ColorPopup : public Node {
  enum class Drag { None, Sv, Hue, Alpha };

 public:
  ColorPopup(ColorPicker* owner, ColorF color, ColorPickerMode mode, bool alpha)
      : owner_(owner),
        color_(ClampColor(color)),
        mode_(mode),
        alpha_(alpha),
        hsv_(RgbToHsv(color_)) {
    hug_width();
    hug_height();
  }

  SizeF Measure(float max_w, float max_h) override {
    if (mode_ == ColorPickerMode::Simple) {
      const float grid_w = kSimpleCols * kSwatch + (kSimpleCols - 1.f) * kGap;
      const float grid_h = kSimpleRows * kSwatch + (kSimpleRows - 1.f) * kGap;
      const float w = kPad * 2.f + grid_w;
      const float h = kPad * 2.f + kPreview + kGap + grid_h;
      return ResolveSize(max_w, max_h, w, h);
    }
    const float preset_w =
        8.f * kSwatch + 7.f * kGap;  // one row of 8 for full
    const float inner = (std::max)(kSvW, preset_w);
    const float w = kPad * 2.f + inner;
    float h = kPad + kSvH + kGap + kBarH + kGap;
    if (alpha_) {
      h += kBarH + kGap;
    }
    h += kPreview + kGap + kSwatch + kPad;
    return ResolveSize(max_w, max_h, w, h);
  }

  void Paint(auralite::Canvas& canvas) override {
    if (!visible()) {
      return;
    }
    const ThemeTokens& th = Theme::Active();
    canvas.FillRoundedRect(bounds_, 8.f, 8.f, th.surface);
    canvas.DrawRect(bounds_, th.border, 1.f);
    if (mode_ == ColorPickerMode::Simple) {
      PaintSimple(canvas, th);
    } else {
      PaintFull(canvas, th);
    }
  }

  void OnMouseMove(const MouseEvent& e) override {
    if (drag_ != Drag::None) {
      ApplyDrag(e.x, e.y, false);
      return;
    }
    int hover = HitPreset(e.x, e.y);
    int zone = 0;
    if (mode_ == ColorPickerMode::Full) {
      if (ContainsPoint(SvRect(), e.x, e.y)) {
        zone = 1;
      } else if (ContainsPoint(HueRect(), e.x, e.y)) {
        zone = 2;
      } else if (alpha_ && ContainsPoint(AlphaRect(), e.x, e.y)) {
        zone = 3;
      }
    }
    if (hover != hover_preset_ || zone != hover_zone_) {
      hover_preset_ = hover;
      hover_zone_ = zone;
      Invalidate();
    }
  }

  void OnMouseLeave(const MouseEvent&) override {
    if (hover_preset_ != -1 || hover_zone_ != 0) {
      hover_preset_ = -1;
      hover_zone_ = 0;
      Invalidate();
    }
  }

  void OnMouseDown(const MouseEvent& e) override {
    if (e.button != MouseButton::Left || !owner_) {
      return;
    }
    const int preset = HitPreset(e.x, e.y);
    if (preset >= 0) {
      ColorF c = kPresets[preset];
      c.a = alpha_ ? color_.a : 1.f;
      hsv_ = RgbToHsv(c);
      CommitColor(c, mode_ == ColorPickerMode::Simple);
      return;
    }
    if (mode_ == ColorPickerMode::Full) {
      if (ContainsPoint(SvRect(), e.x, e.y)) {
        drag_ = Drag::Sv;
        ApplyDrag(e.x, e.y, false);
        return;
      }
      if (ContainsPoint(HueRect(), e.x, e.y)) {
        drag_ = Drag::Hue;
        ApplyDrag(e.x, e.y, false);
        return;
      }
      if (alpha_ && ContainsPoint(AlphaRect(), e.x, e.y)) {
        drag_ = Drag::Alpha;
        ApplyDrag(e.x, e.y, false);
        return;
      }
    }
  }

  void OnMouseUp(const MouseEvent& e) override {
    if (e.button == MouseButton::Left) {
      drag_ = Drag::None;
    }
  }

 private:
  RectF Content() const {
    return RectF{bounds_.x + kPad, bounds_.y + kPad,
                 std::max(0.f, bounds_.w - kPad * 2.f),
                 std::max(0.f, bounds_.h - kPad * 2.f)};
  }

  RectF PreviewRect() const {
    const RectF c = Content();
    if (mode_ == ColorPickerMode::Simple) {
      return RectF{c.x, c.y, c.w, kPreview};
    }
    float y = c.y + kSvH + kGap + kBarH + kGap;
    if (alpha_) {
      y += kBarH + kGap;
    }
    return RectF{c.x, y, kPreview * 1.4f, kPreview};
  }

  RectF HexRect() const {
    const RectF p = PreviewRect();
    return RectF{p.x + p.w + kGap, p.y, std::max(0.f, Content().w - p.w - kGap),
                 p.h};
  }

  RectF SvRect() const {
    const RectF c = Content();
    return RectF{c.x, c.y, kSvW, kSvH};
  }

  RectF HueRect() const {
    const RectF sv = SvRect();
    return RectF{sv.x, sv.y + sv.h + kGap, sv.w, kBarH};
  }

  RectF AlphaRect() const {
    const RectF hue = HueRect();
    return RectF{hue.x, hue.y + hue.h + kGap, hue.w, kBarH};
  }

  RectF PresetGrid() const {
    const RectF c = Content();
    if (mode_ == ColorPickerMode::Simple) {
      return RectF{c.x, c.y + kPreview + kGap,
                   kSimpleCols * kSwatch + (kSimpleCols - 1.f) * kGap,
                   kSimpleRows * kSwatch + (kSimpleRows - 1.f) * kGap};
    }
    const RectF p = PreviewRect();
    return RectF{c.x, p.y + p.h + kGap,
                 8.f * kSwatch + 7.f * kGap, kSwatch};
  }

  int PresetColumns() const {
    return mode_ == ColorPickerMode::Simple ? 8 : 8;
  }

  int PresetRows() const {
    return mode_ == ColorPickerMode::Simple ? 3 : 1;
  }

  int PresetShown() const {
    return (std::min)(kPresetCount, PresetColumns() * PresetRows());
  }

  RectF PresetAt(int index) const {
    const RectF g = PresetGrid();
    const int cols = PresetColumns();
    const int col = index % cols;
    const int row = index / cols;
    return RectF{g.x + col * (kSwatch + kGap), g.y + row * (kSwatch + kGap),
                 kSwatch, kSwatch};
  }

  int HitPreset(float x, float y) const {
    const int n = PresetShown();
    for (int i = 0; i < n; ++i) {
      if (ContainsPoint(PresetAt(i), x, y)) {
        return i;
      }
    }
    return -1;
  }

  void ApplyDrag(float x, float y, bool /*close*/) {
    if (drag_ == Drag::Sv) {
      const RectF r = SvRect();
      hsv_.s = (r.w > 0.f) ? Clamp01((x - r.x) / r.w) : 0.f;
      hsv_.v = (r.h > 0.f) ? Clamp01(1.f - (y - r.y) / r.h) : 0.f;
    } else if (drag_ == Drag::Hue) {
      const RectF r = HueRect();
      hsv_.h = (r.w > 0.f) ? Clamp01((x - r.x) / r.w) * 360.f : 0.f;
    } else if (drag_ == Drag::Alpha) {
      const RectF r = AlphaRect();
      color_.a = (r.w > 0.f) ? Clamp01((x - r.x) / r.w) : 1.f;
      CommitColor(HsvToRgb(hsv_.h, hsv_.s, hsv_.v, color_.a), false);
      return;
    } else {
      return;
    }
    CommitColor(HsvToRgb(hsv_.h, hsv_.s, hsv_.v, color_.a), false);
  }

  void CommitColor(const ColorF& c, bool close) {
    color_ = ClampColor(c);
    hsv_ = RgbToHsv(color_);
    if (owner_) {
      owner_->Commit(color_, close);
    }
    Invalidate();
  }

  void PaintSimple(auralite::Canvas& canvas, const ThemeTokens& th) {
    PaintPreview(canvas, th);
    PaintPresets(canvas, th);
  }

  void PaintFull(auralite::Canvas& canvas, const ThemeTokens& th) {
    const RectF sv = SvRect();
    // SV: columns of hue→white, rows toward black.
    constexpr int kCols = 24;
    constexpr int kRows = 16;
    for (int row = 0; row < kRows; ++row) {
      for (int col = 0; col < kCols; ++col) {
        const float s = (col + 0.5f) / kCols;
        const float v = 1.f - (row + 0.5f) / kRows;
        const float x = sv.x + col * (sv.w / kCols);
        const float y = sv.y + row * (sv.h / kRows);
        const float w = sv.w / kCols + 0.5f;
        const float h = sv.h / kRows + 0.5f;
        canvas.FillRect(RectF{x, y, w, h}, HsvToRgb(hsv_.h, s, v, 1.f));
      }
    }
    canvas.DrawRect(sv, th.border, 1.f);
    const float cx = sv.x + hsv_.s * sv.w;
    const float cy = sv.y + (1.f - hsv_.v) * sv.h;
    canvas.DrawEllipse(RectF{cx - 5.f, cy - 5.f, 10.f, 10.f},
                       ColorF::FromRgb(255, 255, 255), 2.f);
    canvas.DrawEllipse(RectF{cx - 4.f, cy - 4.f, 8.f, 8.f},
                       ColorF::FromRgb(0, 0, 0), 1.f);

    const RectF hue = HueRect();
    constexpr int kHueSeg = 36;
    for (int i = 0; i < kHueSeg; ++i) {
      const float t0 = static_cast<float>(i) / kHueSeg;
      const float tw = hue.w / kHueSeg + 0.5f;
      canvas.FillRect(RectF{hue.x + t0 * hue.w, hue.y, tw, hue.h},
                      HueColor(t0 * 360.f));
    }
    canvas.DrawRect(hue, th.border, 1.f);
    const float hx = hue.x + (hsv_.h / 360.f) * hue.w;
    canvas.FillRect(RectF{hx - 1.5f, hue.y - 1.f, 3.f, hue.h + 2.f},
                    ColorF::FromRgb(255, 255, 255));
    canvas.DrawRect(RectF{hx - 1.5f, hue.y - 1.f, 3.f, hue.h + 2.f},
                    ColorF::FromRgb(0, 0, 0), 1.f);

    if (alpha_) {
      const RectF ar = AlphaRect();
      FillChecker(canvas, ar, 5.f);
      constexpr int kASeg = 24;
      for (int i = 0; i < kASeg; ++i) {
        const float t0 = static_cast<float>(i) / kASeg;
        ColorF solid = color_;
        solid.a = t0 + 0.5f / kASeg;
        canvas.FillRect(RectF{ar.x + t0 * ar.w, ar.y, ar.w / kASeg + 0.5f, ar.h},
                        solid);
      }
      canvas.DrawRect(ar, th.border, 1.f);
      const float ax = ar.x + color_.a * ar.w;
      canvas.FillRect(RectF{ax - 1.5f, ar.y - 1.f, 3.f, ar.h + 2.f},
                      ColorF::FromRgb(255, 255, 255));
      canvas.DrawRect(RectF{ax - 1.5f, ar.y - 1.f, 3.f, ar.h + 2.f},
                      ColorF::FromRgb(0, 0, 0), 1.f);
    }

    PaintPreview(canvas, th);
    PaintPresets(canvas, th);
  }

  void PaintPreview(auralite::Canvas& canvas, const ThemeTokens& th) {
    const RectF p = PreviewRect();
    FillChecker(canvas, p, 6.f);
    canvas.FillRoundedRect(p, 4.f, 4.f, color_);
    canvas.DrawRoundedRect(p, 4.f, 4.f, th.border, 1.f);
    canvas.DrawText(FormatHex(color_, alpha_), HexRect(), th.text, 13.f,
                    th.font_ui.c_str(), auralite::TextHAlign::Left);
  }

  void PaintPresets(auralite::Canvas& canvas, const ThemeTokens& th) {
    const int n = PresetShown();
    for (int i = 0; i < n; ++i) {
      const RectF r = PresetAt(i);
      canvas.FillRoundedRect(r, 3.f, 3.f, kPresets[i]);
      const bool sel = ColorsEqual(kPresets[i],
                                   ColorF{color_.r, color_.g, color_.b, 1.f}) ||
                       (hover_preset_ == i);
      canvas.DrawRoundedRect(r, 3.f, 3.f,
                             sel ? th.border_focus : th.border,
                             sel ? 1.5f : 1.f);
    }
  }

  ColorPicker* owner_ = nullptr;
  ColorF color_;
  ColorPickerMode mode_ = ColorPickerMode::Simple;
  bool alpha_ = false;
  Hsv hsv_;
  Drag drag_ = Drag::None;
  int hover_preset_ = -1;
  int hover_zone_ = 0;
};

ColorPicker::ColorPicker() {
  set_focusable(true);
  fill_width();
  fixed_height(36.f);
}

void ColorPicker::BindWindow(Window* window) { window_ = window; }

ColorPicker& ColorPicker::color(const ColorF& c) {
  const ColorF next = ClampColor(c);
  if (ColorsEqual(next, color_)) {
    return *this;
  }
  color_ = next;
  NotifyAccValueChanged();
  Invalidate();
  return *this;
}

bool ColorPicker::set_hex(const std::wstring& hex) {
  ColorF parsed;
  if (!ParseHexWide(hex, &parsed)) {
    return false;
  }
  if (!alpha_) {
    parsed.a = 1.f;
  }
  color(parsed);
  return true;
}

std::wstring ColorPicker::hex(bool with_alpha) const {
  return FormatHex(color_, with_alpha || alpha_);
}

ColorPicker& ColorPicker::mode(ColorPickerMode m) {
  mode_ = m;
  return *this;
}

ColorPicker& ColorPicker::alpha(bool enable) {
  alpha_ = enable;
  Invalidate();
  return *this;
}

ColorPicker& ColorPicker::font_size(float size) {
  font_size_ = size;
  return *this;
}

ColorPicker& ColorPicker::on_changed(ChangeHandler handler) {
  on_changed_ = std::move(handler);
  return *this;
}

void ColorPicker::Notify() {
  if (on_changed_) {
    on_changed_(color_);
  }
}

void ColorPicker::Commit(const ColorF& c, bool close) {
  color_ = ClampColor(c);
  if (!alpha_) {
    color_.a = 1.f;
  }
  NotifyAccValueChanged();
  Notify();
  Invalidate();
  if (close) {
    const bool was = open_;
    open_ = false;
    if (window_) {
      window_->RequestClearPopup();
    }
    if (was) {
      NotifyAccExpandCollapseChanged();
    }
  }
}

std::wstring ColorPicker::DisplayText() const { return hex(alpha_); }

void ColorPicker::ClosePopup() {
  const bool was = open_;
  open_ = false;
  if (window_ && window_->popup()) {
    window_->ClearPopup();
  }
  if (was) {
    NotifyAccExpandCollapseChanged();
  }
}

void ColorPicker::OpenPopup() {
  if (!window_) {
    return;
  }
  if (open_) {
    ClosePopup();
    return;
  }
  auto pop =
      std::make_unique<ColorPopup>(this, color_, mode_, alpha_);
  const SizeF want = pop->Measure(320.f, 400.f);
  pop->Layout(RectF{bounds_.x, bounds_.y + bounds_.h + 2.f, want.w, want.h});
  open_ = true;
  window_->SetPopup(
      std::move(pop),
      [this]() {
        const bool was = open_;
        open_ = false;
        if (was) {
          NotifyAccExpandCollapseChanged();
        }
      },
      this);
  window_->SetFocusNode(this);
  NotifyAccExpandCollapseChanged();
}

AccRole ColorPicker::acc_role() const {
  if (acc_role_override_) {
    return *acc_role_override_;
  }
  return AccRole::ComboBox;
}

std::wstring ColorPicker::AccDefaultName() const { return {}; }

std::wstring ColorPicker::AccValue() const { return DisplayText(); }

bool ColorPicker::AccSetValue(const std::wstring& value) {
  ColorF parsed;
  if (!ParseHexWide(value, &parsed)) {
    return false;
  }
  Commit(parsed, false);
  return true;
}

bool ColorPicker::AccIsExpanded() const { return open_; }

bool ColorPicker::AccExpand() {
  if (open_) {
    return true;
  }
  OpenPopup();
  return open_;
}

bool ColorPicker::AccCollapse() {
  if (!open_) {
    return true;
  }
  ClosePopup();
  return true;
}

SizeF ColorPicker::Measure(float max_w, float max_h) {
  return ResolveSize(max_w, max_h,
                     preferred_width() > 0.f ? preferred_width() : max_w, 36.f);
}

void ColorPicker::Paint(auralite::Canvas& canvas) {
  if (!visible()) {
    return;
  }
  const ThemeTokens& th = Theme::Active();
  canvas.FillRoundedRect(bounds_, 6.f, 6.f, th.surface);
  canvas.DrawRect(bounds_, focused() || open_ ? th.border_focus : th.border,
                  focused() || open_ ? 1.5f : 1.f);

  const RectF swatch{bounds_.x + 8.f, bounds_.y + 7.f, 22.f, 22.f};
  FillChecker(canvas, swatch, 5.f);
  canvas.FillRoundedRect(swatch, 4.f, 4.f, color_);
  canvas.DrawRoundedRect(swatch, 4.f, 4.f, th.border, 1.f);

  const RectF text{swatch.x + swatch.w + 8.f, bounds_.y,
                   std::max(0.f, bounds_.w - 36.f - (swatch.w + 16.f)),
                   bounds_.h};
  canvas.DrawText(DisplayText(), text, th.text, ResolveFontSize(font_size_),
                  th.font_ui.c_str(), auralite::TextHAlign::Left);

  const float cx = bounds_.x + bounds_.w - 18.f;
  const float cy = bounds_.y + bounds_.h * 0.5f;
  if (open_) {
    canvas.DrawLine(cx - 5.f, cy + 2.f, cx, cy - 3.f, th.glyph, 1.5f);
    canvas.DrawLine(cx, cy - 3.f, cx + 5.f, cy + 2.f, th.glyph, 1.5f);
  } else {
    canvas.DrawLine(cx - 5.f, cy - 2.f, cx, cy + 3.f, th.glyph, 1.5f);
    canvas.DrawLine(cx, cy + 3.f, cx + 5.f, cy - 2.f, th.glyph, 1.5f);
  }
}

void ColorPicker::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  OpenPopup();
}

void ColorPicker::OnKey(const KeyEvent& e) {
  if (!e.down) {
    return;
  }
  if (e.vk == VK_ESCAPE && open_) {
    ClosePopup();
    return;
  }
  if (e.vk == VK_SPACE || e.vk == VK_DOWN || e.vk == VK_RETURN) {
    OpenPopup();
  }
}

}  // namespace auralite::ui
