// Console tests for Theme ParseColorHex / Register / SetActive / merge refresh.
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <fstream>
#include <string>

#include "mx/ui/theme.h"
#include "mx/ui/theme_yaml.h"

namespace {

int g_failures = 0;

void Expect(bool cond, const char* name) {
  if (!cond) {
    std::printf("FAIL %s\n", name);
    ++g_failures;
  } else {
    std::printf("ok   %s\n", name);
  }
}

bool ColorNear(const mx::ColorF& c, float r, float g, float b, float a,
               float eps = 0.01f) {
  return std::fabs(c.r - r) < eps && std::fabs(c.g - g) < eps &&
         std::fabs(c.b - b) < eps && std::fabs(c.a - a) < eps;
}

void TestParseColorHex() {
  using mx::ColorF;
  using mx::ui::ParseColorHex;

  ColorF c{};
  ColorF sentinel = ColorF::FromRgb(1, 2, 3, 4);
  c = sentinel;
  Expect(!ParseColorHex("RRGGBB", &c), "parse missing hash");
  Expect(ColorNear(c, 1.f / 255.f, 2.f / 255.f, 3.f / 255.f, 4.f / 255.f),
         "parse fail leaves out");

  Expect(ParseColorHex("#286ec8", &c), "parse rgb");
  Expect(ColorNear(c, 40.f / 255.f, 110.f / 255.f, 200.f / 255.f, 1.f),
         "parse rgb values");

  Expect(ParseColorHex("#3399ff8c", &c), "parse rgba");
  Expect(ColorNear(c, 51.f / 255.f, 153.f / 255.f, 255.f / 255.f, 140.f / 255.f),
         "parse rgba values");

  Expect(!ParseColorHex("#xyz", &c), "parse bad hex");
  Expect(!ParseColorHex("#12345", &c), "parse wrong len");
  Expect(!ParseColorHex("#286ec8", nullptr), "parse null out");
}

void TestRegisterSetActive() {
  using namespace mx::ui;

  Expect(!Theme::Register("", MakeBuiltInLightTokens()), "reject empty name");

  Theme::RegisterBuiltInLight();
  Theme::RegisterBuiltInDark();
  Expect(Theme::SetActive("light"), "set light");
  Expect(Theme::ActiveName() == "light", "active name light");
  Expect(Theme::Active().font_size == 14.f, "light font size");

  Expect(Theme::SetActive("dark"), "set dark");
  Expect(Theme::ActiveName() == "dark", "active name dark");
  Expect(ColorNear(Theme::Active().danger, 130.f / 255.f, 48.f / 255.f, 48.f / 255.f,
                   1.f),
         "builtin dark danger");
  Expect(ColorNear(Theme::Active().warning, 210.f / 255.f, 160.f / 255.f, 60.f / 255.f,
                   1.f),
         "builtin dark warning");
  Expect(!Theme::SetActive("nope"), "unknown theme");
  Expect(Theme::ActiveName() == "dark", "still dark after fail");

  ThemeTokens patched = MakeBuiltInDarkTokens();
  patched.accent = mx::ColorF::FromRgb(255, 0, 0);
  Expect(Theme::Register("dark", patched), "re-register active");
  Expect(ColorNear(Theme::Active().accent, 1.f, 0.f, 0.f, 1.f),
         "active refreshed on re-register");
}

void TestRegisterFromFileMerge() {
  using namespace mx::ui;

  const char* path = "theme_test_overlay.yaml";
  {
    std::ofstream out(path);
    out << "name: dark\ncolors:\n  accent: \"#00ff00\"\n  bogush: \"#zzzzzz\"\n"
           "fonts:\n  size: -3\n";
  }

  Theme::RegisterBuiltInDark();
  Theme::SetActive("dark");
  const float prev_size = Theme::Active().font_size;
  Expect(Theme::RegisterFromFile(path), "register overlay file");
  Expect(Theme::SetActive("dark"), "reactivate dark");
  Expect(ColorNear(Theme::Active().accent, 0.f, 1.f, 0.f, 1.f),
         "overlay accent");
  Expect(Theme::Active().font_size == prev_size, "negative size ignored");

  std::remove(path);

  {
    std::ofstream out(path);
    out << "name: broken\ncolors:\n  accent: [1, 2, 3]\n";
  }
  // accent type mismatch should skip field; name ok → still register
  Expect(Theme::RegisterFromFile(path), "register with bad accent type");
  std::remove(path);

  {
    std::ofstream out(path);
    out << "colors:\n  accent: \"#00ff00\"\n";  // missing name
  }
  Expect(!Theme::RegisterFromFile(path), "reject missing name");
  std::remove(path);
}

void TestWindowScope() {
  using namespace mx::ui;

  Theme::RegisterBuiltInLight();
  Theme::RegisterBuiltInDark();
  Expect(Theme::SetActive("light"), "scope process light");
  Expect(Theme::ActiveName() == "light", "scope name light");
  Expect(ColorNear(Theme::Active().window_bg, 245.f / 255.f, 248.f / 255.f,
                   252.f / 255.f, 1.f),
         "process light window_bg");

  {
    Theme::Scope dark("dark");
    Expect(Theme::ActiveName() == "dark", "scope name dark");
    Expect(ColorNear(Theme::Active().window_bg, 30.f / 255.f, 30.f / 255.f,
                     30.f / 255.f, 1.f),
           "scope dark window_bg");
    {
      Theme::Scope inherit("");
      Expect(Theme::ActiveName() == "light", "empty inherits process");
      Expect(ColorNear(Theme::Active().window_bg, 245.f / 255.f, 248.f / 255.f,
                       252.f / 255.f, 1.f),
             "empty barrier uses process bg");
    }
    Expect(Theme::ActiveName() == "dark", "restored after empty");
    {
      Theme::Scope unknown("nope");
      Expect(Theme::ActiveName() == "dark", "unknown skipped");
    }
  }
  Expect(Theme::ActiveName() == "light", "restored process name");
  Expect(ColorNear(Theme::Active().window_bg, 245.f / 255.f, 248.f / 255.f,
                   252.f / 255.f, 1.f),
         "restored process bg");
}

}  // namespace

int main() {
  TestParseColorHex();
  TestRegisterSetActive();
  TestRegisterFromFileMerge();
  TestWindowScope();
  if (g_failures > 0) {
    std::printf("%d failure(s)\n", g_failures);
    return 1;
  }
  std::printf("all theme tests passed\n");
  return 0;
}
