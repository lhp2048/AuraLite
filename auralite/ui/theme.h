#pragma once

#include "auralite/canvas.h"  // ColorF
#include <functional>
#include <optional>
#include <string>

namespace auralite::ui {

struct ThemeTokens {
  ColorF window_bg;
  ColorF surface;
  ColorF surface_alt;
  ColorF text;
  ColorF text_muted;
  ColorF text_on_accent;
  ColorF accent;
  ColorF accent_hover;
  ColorF accent_pressed;
  ColorF accent_soft;
  ColorF border;
  ColorF border_focus;
  ColorF divider;
  ColorF selection;
  ColorF scroll_track;
  ColorF scroll_thumb;
  ColorF glyph;
  std::wstring font_ui;
  float font_size = 14.f;
  float font_size_sm = 13.f;
};

class Theme {
 public:
  static const ThemeTokens& Active();
  static const std::string& ActiveName();

  static void RegisterBuiltInLight();
  static void RegisterBuiltInDark();
  // Empty name → false. Re-registering the active theme refreshes Active + sinks.
  static bool Register(std::string name, ThemeTokens tokens);
  static bool Has(const std::string& name);
  static bool Get(const std::string& name, ThemeTokens* out);
  static bool RegisterFromFile(const std::string& path);
  static bool RegisterFromDir(const std::string& dir);
  static bool SetActive(const std::string& name);

  using InvalidateSink = std::function<void()>;
  static void AddInvalidateSink(InvalidateSink* sink);
  static void RemoveInvalidateSink(InvalidateSink* sink);
};

ThemeTokens MakeBuiltInLightTokens();
ThemeTokens MakeBuiltInDarkTokens();

// Control override: unset → Theme::Active().font_size
inline float ResolveFontSize(const std::optional<float>& override_px) {
  return override_px.value_or(Theme::Active().font_size);
}

}  // namespace auralite::ui
