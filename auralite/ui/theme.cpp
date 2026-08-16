#include "auralite/ui/theme.h"

#include <algorithm>
#include <unordered_map>
#include <vector>

namespace auralite::ui {

namespace {

std::unordered_map<std::string, ThemeTokens> g_registry;
ThemeTokens g_active;
std::string g_active_name;
std::vector<Theme::InvalidateSink*> g_sinks;

void NotifySinks() {
  for (Theme::InvalidateSink* sink : g_sinks) {
    if (sink && *sink) {
      (*sink)();
    }
  }
}

void EnsureDefaultTheme() {
  if (!g_active_name.empty()) {
    return;
  }
  Theme::RegisterBuiltInLight();
  Theme::SetActive("light");
}

}  // namespace

ThemeTokens MakeBuiltInLightTokens() {
  ThemeTokens t;
  t.window_bg = ColorF::FromRgb(245, 248, 252);
  t.surface = ColorF::FromRgb(255, 255, 255);
  t.surface_alt = ColorF::FromRgb(245, 247, 250);
  t.text = ColorF::FromRgb(25, 35, 50);
  t.text_muted = ColorF::FromRgb(150, 160, 175);
  t.text_on_accent = ColorF::FromRgb(255, 255, 255);
  t.accent = ColorF::FromRgb(40, 110, 200);
  t.accent_hover = ColorF::FromRgb(55, 130, 215);
  t.accent_pressed = ColorF::FromRgb(25, 85, 160);
  t.accent_soft = ColorF::FromRgb(230, 238, 250);
  t.border = ColorF::FromRgb(170, 180, 195);
  t.border_focus = ColorF::FromRgb(40, 110, 200);
  t.divider = ColorF::FromRgb(230, 233, 238);
  t.selection = ColorF::FromRgb(51, 153, 255, 140);
  t.scroll_track = ColorF::FromRgb(220, 220, 220);
  t.scroll_thumb = ColorF::FromRgb(150, 150, 150);
  t.glyph = ColorF::FromRgb(40, 110, 200);
  t.danger = ColorF::FromRgb(180, 50, 50);
  t.danger_hover = ColorF::FromRgb(200, 70, 70);
  t.danger_pressed = ColorF::FromRgb(140, 40, 40);
  t.warning = ColorF::FromRgb(210, 160, 60);
  t.font_ui = L"Microsoft YaHei UI";
  t.font_size = 14.f;
  t.font_size_sm = 13.f;
  return t;
}

ThemeTokens MakeBuiltInDarkTokens() {
  ThemeTokens t;
  t.window_bg = ColorF::FromRgb(30, 30, 30);
  t.surface = ColorF::FromRgb(45, 45, 45);
  t.surface_alt = ColorF::FromRgb(37, 37, 37);
  t.text = ColorF::FromRgb(232, 232, 232);
  t.text_muted = ColorF::FromRgb(154, 154, 154);
  t.text_on_accent = ColorF::FromRgb(255, 255, 255);
  t.accent = ColorF::FromRgb(76, 139, 245);
  t.accent_hover = ColorF::FromRgb(93, 151, 247);
  t.accent_pressed = ColorF::FromRgb(58, 122, 224);
  t.accent_soft = ColorF::FromRgb(42, 58, 80);
  t.border = ColorF::FromRgb(85, 85, 85);
  t.border_focus = ColorF::FromRgb(76, 139, 245);
  t.divider = ColorF::FromRgb(64, 64, 64);
  t.selection = ColorF::FromRgb(76, 139, 245, 136);
  t.scroll_track = ColorF::FromRgb(58, 58, 58);
  t.scroll_thumb = ColorF::FromRgb(136, 136, 136);
  t.glyph = ColorF::FromRgb(76, 139, 245);
  t.danger = ColorF::FromRgb(130, 48, 48);
  t.danger_hover = ColorF::FromRgb(160, 60, 60);
  t.danger_pressed = ColorF::FromRgb(100, 36, 36);
  t.warning = ColorF::FromRgb(210, 160, 60);
  t.font_ui = L"Microsoft YaHei UI";
  t.font_size = 14.f;
  t.font_size_sm = 13.f;
  return t;
}

const ThemeTokens& Theme::Active() {
  EnsureDefaultTheme();
  return g_active;
}

const std::string& Theme::ActiveName() {
  return g_active_name;
}

void Theme::RegisterBuiltInLight() {
  g_registry["light"] = MakeBuiltInLightTokens();
}

void Theme::RegisterBuiltInDark() {
  g_registry["dark"] = MakeBuiltInDarkTokens();
}

bool Theme::Register(std::string name, ThemeTokens tokens) {
  if (name.empty()) {
    return false;
  }
  g_registry[name] = tokens;
  if (name == g_active_name) {
    g_active = tokens;
    NotifySinks();
  }
  return true;
}

bool Theme::Has(const std::string& name) {
  return g_registry.find(name) != g_registry.end();
}

bool Theme::Get(const std::string& name, ThemeTokens* out) {
  if (out == nullptr) {
    return false;
  }
  const auto it = g_registry.find(name);
  if (it == g_registry.end()) {
    return false;
  }
  *out = it->second;
  return true;
}

bool Theme::SetActive(const std::string& name) {
  const auto it = g_registry.find(name);
  if (it == g_registry.end()) {
    return false;
  }
  g_active = it->second;
  g_active_name = name;
  NotifySinks();
  return true;
}

void Theme::AddInvalidateSink(InvalidateSink* sink) {
  if (sink == nullptr) {
    return;
  }
  if (std::find(g_sinks.begin(), g_sinks.end(), sink) != g_sinks.end()) {
    return;
  }
  g_sinks.push_back(sink);
}

void Theme::RemoveInvalidateSink(InvalidateSink* sink) {
  if (sink == nullptr) {
    return;
  }
  const auto it = std::find(g_sinks.begin(), g_sinks.end(), sink);
  if (it != g_sinks.end()) {
    g_sinks.erase(it);
  }
}

}  // namespace auralite::ui
