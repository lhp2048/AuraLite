#include "auralite/ui/theme_yaml.h"

#include "auralite/ui/theme.h"
#include "auralite/ui/yaml_loader.h"

#include <yaml-cpp/yaml.h>

#include <cctype>
#include <filesystem>
#include <string>

namespace auralite::ui {

namespace {

bool ParseHexByte(const char* p, uint8_t* out) {
  auto hexVal = [](char c) -> int {
    if (c >= '0' && c <= '9') {
      return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
      return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
      return c - 'A' + 10;
    }
    return -1;
  };
  const int hi = hexVal(p[0]);
  const int lo = hexVal(p[1]);
  if (hi < 0 || lo < 0) {
    return false;
  }
  *out = static_cast<uint8_t>((hi << 4) | lo);
  return true;
}

template <typename T>
bool TryAs(const YAML::Node& node, T* out) {
  try {
    *out = node.as<T>();
    return true;
  } catch (const YAML::Exception&) {
    return false;
  }
}

void ApplyColorField(const YAML::Node& colors, const char* key, ColorF* field) {
  if (!colors || !colors[key]) {
    return;
  }
  std::string hex;
  if (!TryAs(colors[key], &hex)) {
    return;
  }
  ColorF parsed;
  if (!ParseColorHex(hex, &parsed)) {
    return;
  }
  *field = parsed;
}

void ApplyThemeYaml(const YAML::Node& root, ThemeTokens* tokens) {
  if (const YAML::Node colors = root["colors"]) {
    ApplyColorField(colors, "window_bg", &tokens->window_bg);
    ApplyColorField(colors, "surface", &tokens->surface);
    ApplyColorField(colors, "surface_alt", &tokens->surface_alt);
    ApplyColorField(colors, "text", &tokens->text);
    ApplyColorField(colors, "text_muted", &tokens->text_muted);
    ApplyColorField(colors, "text_on_accent", &tokens->text_on_accent);
    ApplyColorField(colors, "accent", &tokens->accent);
    ApplyColorField(colors, "accent_hover", &tokens->accent_hover);
    ApplyColorField(colors, "accent_pressed", &tokens->accent_pressed);
    ApplyColorField(colors, "accent_soft", &tokens->accent_soft);
    ApplyColorField(colors, "border", &tokens->border);
    ApplyColorField(colors, "border_focus", &tokens->border_focus);
    ApplyColorField(colors, "divider", &tokens->divider);
    ApplyColorField(colors, "selection", &tokens->selection);
    ApplyColorField(colors, "scroll_track", &tokens->scroll_track);
    ApplyColorField(colors, "scroll_thumb", &tokens->scroll_thumb);
    ApplyColorField(colors, "glyph", &tokens->glyph);
    ApplyColorField(colors, "danger", &tokens->danger);
    ApplyColorField(colors, "danger_hover", &tokens->danger_hover);
    ApplyColorField(colors, "danger_pressed", &tokens->danger_pressed);
    ApplyColorField(colors, "warning", &tokens->warning);
  }

  if (const YAML::Node fonts = root["fonts"]) {
    if (fonts["ui"]) {
      std::string font_ui;
      if (TryAs(fonts["ui"], &font_ui)) {
        tokens->font_ui = Utf8ToWide(font_ui);
      }
    }
    if (fonts["size"]) {
      float font_size = 0.0f;
      if (TryAs(fonts["size"], &font_size) && font_size > 0.f) {
        tokens->font_size = font_size;
      }
    }
    if (fonts["size_sm"]) {
      float font_size_sm = 0.0f;
      if (TryAs(fonts["size_sm"], &font_size_sm) && font_size_sm > 0.f) {
        tokens->font_size_sm = font_size_sm;
      }
    }
  }
}

bool IsYamlExtension(const std::filesystem::path& path) {
  std::string ext = path.extension().string();
  for (char& c : ext) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }
  return ext == ".yaml" || ext == ".yml";
}

}  // namespace

bool ParseColorHex(const std::string& s, ColorF* out) {
  if (out == nullptr || s.empty() || s[0] != '#') {
    return false;
  }

  const size_t hex_len = s.size() - 1;
  if (hex_len != 6 && hex_len != 8) {
    return false;
  }

  for (size_t i = 1; i < s.size(); ++i) {
    if (!std::isxdigit(static_cast<unsigned char>(s[i]))) {
      return false;
    }
  }

  uint8_t rr = 0;
  uint8_t gg = 0;
  uint8_t bb = 0;
  uint8_t aa = 255;
  if (!ParseHexByte(s.data() + 1, &rr) ||
      !ParseHexByte(s.data() + 3, &gg) ||
      !ParseHexByte(s.data() + 5, &bb)) {
    return false;
  }
  if (hex_len == 8 && !ParseHexByte(s.data() + 7, &aa)) {
    return false;
  }

  *out = ColorF::FromRgb(rr, gg, bb, aa);
  return true;
}

bool Theme::RegisterFromFile(const std::string& path) {
  try {
    const YAML::Node root = YAML::LoadFile(path);
    if (!root || !root.IsMap() || !root["name"]) {
      return false;
    }

    const std::string name = root["name"].as<std::string>();
    if (name.empty()) {
      return false;
    }

    ThemeTokens tokens =
        Theme::Has(name) ? [&]() {
          ThemeTokens existing;
          Theme::Get(name, &existing);
          return existing;
        }()
                         : MakeBuiltInLightTokens();

    ApplyThemeYaml(root, &tokens);
    return Theme::Register(name, std::move(tokens));
  } catch (const YAML::Exception&) {
    return false;
  }
}

bool Theme::RegisterFromDir(const std::string& dir) {
  std::error_code ec;
  const std::filesystem::path dir_path(dir);
  if (!std::filesystem::is_directory(dir_path, ec)) {
    return false;
  }

  int succeeded = 0;
  for (const auto& entry :
       std::filesystem::directory_iterator(dir_path, ec)) {
    if (ec) {
      break;
    }
    if (!entry.is_regular_file(ec)) {
      continue;
    }
    if (!IsYamlExtension(entry.path())) {
      continue;
    }
    if (RegisterFromFile(entry.path().string())) {
      ++succeeded;
    }
  }

  return succeeded >= 1;
}

}  // namespace auralite::ui
