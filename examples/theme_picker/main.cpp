#include <windows.h>
#include <objbase.h>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>

#include "auralite/ui.h"

namespace {

using namespace auralite::ui;

constexpr char kCustomTheme[] = "custom";

float Clamp01(float v) { return std::clamp(v, 0.f, 1.f); }

ColorF Mix(const ColorF& a, const ColorF& b, float t) {
  t = Clamp01(t);
  return ColorF{a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t,
                a.b + (b.b - a.b) * t,
                a.a + (b.a - a.a) * t};
}

ColorF AdjustRgb(const ColorF& c, float delta) {
  return ColorF{Clamp01(c.r + delta), Clamp01(c.g + delta),
                Clamp01(c.b + delta), c.a};
}

float Luminance(const ColorF& c) {
  return 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
}

void ApplyAccentFamily(ThemeTokens* t, const ColorF& accent) {
  if (!t) {
    return;
  }
  t->accent = accent;
  t->accent_hover = AdjustRgb(accent, 0.08f);
  t->accent_pressed = AdjustRgb(accent, -0.12f);
  const ColorF soft_base =
      Luminance(accent) > 0.45f ? ColorF::FromRgb(255, 255, 255)
                                  : ColorF::FromRgb(0, 0, 0);
  t->accent_soft = Mix(soft_base, accent, 0.12f);
  t->border_focus = accent;
  t->glyph = accent;
  t->selection = ColorF{accent.r, accent.g, accent.b, 0.55f};
}

struct ThemeEditor {
  Window window;
  ThemeTokens base_;
  bool dark_base_ = false;

  ColorPicker* accent = nullptr;
  ColorPicker* window_bg = nullptr;
  ColorPicker* surface = nullptr;
  ColorPicker* text = nullptr;
  Label* status = nullptr;

  void SetStatus(std::wstring msg) {
    if (status) {
      status->text(std::move(msg));
    }
    window.Invalidate();
  }

  void SyncPickersFromBase() {
    if (accent) {
      accent->color(base_.accent);
    }
    if (window_bg) {
      window_bg->color(base_.window_bg);
    }
    if (surface) {
      surface->color(base_.surface);
    }
    if (text) {
      text->color(base_.text);
    }
  }

  void ApplyCustomTheme() {
    ThemeTokens t = base_;
    if (accent) {
      ApplyAccentFamily(&t, accent->color());
    }
    if (window_bg) {
      t.window_bg = window_bg->color();
    }
    if (surface) {
      t.surface = surface->color();
      t.surface_alt = Mix(t.surface, t.window_bg, 0.35f);
      t.divider = Mix(t.surface, t.text, 0.12f);
      t.border = Mix(t.surface, t.text, 0.28f);
    }
    if (text) {
      t.text = text->color();
      t.text_muted = Mix(t.text, t.surface, 0.55f);
      t.text_on_accent = Luminance(t.text) > 0.6f ? ColorF::FromRgb(0, 0, 0)
                                                  : ColorF::FromRgb(255, 255, 255);
    }
    Theme::Register(kCustomTheme, t);
    Theme::SetActive(kCustomTheme);
    SetStatus(L"主题已应用：" + std::wstring(dark_base_ ? L"深色底" : L"浅色底") +
              L" · accent " + (accent ? accent->hex() : L""));
  }

  void LoadBase(bool dark) {
    dark_base_ = dark;
    base_ = dark ? MakeBuiltInDarkTokens() : MakeBuiltInLightTokens();
    SyncPickersFromBase();
    ApplyCustomTheme();
  }

  void WirePickers() {
    auto hook = [this](const ColorF&) { ApplyCustomTheme(); };
    if (accent) {
      accent->BindWindow(&window);
      accent->on_changed(hook);
    }
    if (window_bg) {
      window_bg->BindWindow(&window);
      window_bg->on_changed(hook);
    }
    if (surface) {
      surface->BindWindow(&window);
      surface->on_changed(hook);
    }
    if (text) {
      text->BindWindow(&window);
      text->on_changed(hook);
    }
  }
};

std::unique_ptr<Node> BuildUi(ThemeEditor* ed) {
  auto accent_b = dsl::ColorPicker();
  accent_b.color(ed->base_.accent)
      .mode(ColorPickerMode::Full)
      .alpha(false);
  ed->accent = accent_b.get();

  auto bg_b = dsl::ColorPicker();
  bg_b.color(ed->base_.window_bg).mode(ColorPickerMode::Simple);
  ed->window_bg = bg_b.get();

  auto surface_b = dsl::ColorPicker();
  surface_b.color(ed->base_.surface).mode(ColorPickerMode::Simple);
  ed->surface = surface_b.get();

  auto text_b = dsl::ColorPicker();
  text_b.color(ed->base_.text).mode(ColorPickerMode::Simple);
  ed->text = text_b.get();

  auto status_b = dsl::Label();
  status_b.text(L"调整 ColorPicker，整窗主题即时刷新。")
      .font_size(13.f)
      .preferred_height(22.f);
  ed->status = status_b.get();

  auto light_b = dsl::Button();
  light_b.text(L"浅色底").hug_width().fixed_height(32.f);
  Button* light = light_b.get();

  auto dark_b = dsl::Button();
  dark_b.text(L"深色底").hug_width().fixed_height(32.f);
  Button* dark = dark_b.get();

  light->on_click([ed] { ed->LoadBase(false); });
  dark->on_click([ed] { ed->LoadBase(true); });

  return dsl::Column()
      .fill_width()
      .fill_height()
      .child(dsl::TitleBar().title(L"主题调色 · ColorPicker"))
      .child(dsl::Column()
                 .fill_width()
                 .fill_height()
                 .padding(20.f)
                 .spacing(10.f)
                 .child(dsl::Label()
                            .text(L"用 ColorPicker 选色，写入 ThemeTokens 并 SetActive。")
                            .font_size(14.f)
                            .preferred_height(22.f))
                 .child(dsl::Label()
                            .text(L"基础模板")
                            .font_size(13.f)
                            .preferred_height(18.f))
                 .child(dsl::Row()
                            .fill_width()
                            .spacing(8.f)
                            .child(std::move(light_b))
                            .child(std::move(dark_b)))
                 .child(dsl::Label()
                            .text(L"强调色 accent（完整模式：SV + 色相）")
                            .font_size(13.f)
                            .preferred_height(18.f))
                 .child(std::move(accent_b))
                 .child(dsl::Label()
                            .text(L"窗口背景 window_bg")
                            .font_size(13.f)
                            .preferred_height(18.f))
                 .child(std::move(bg_b))
                 .child(dsl::Label()
                            .text(L"控件表面 surface")
                            .font_size(13.f)
                            .preferred_height(18.f))
                 .child(std::move(surface_b))
                 .child(dsl::Label()
                            .text(L"文字 text")
                            .font_size(13.f)
                            .preferred_height(18.f))
                 .child(std::move(text_b))
                 .child(dsl::Label()
                            .text(L"预览")
                            .font_size(13.f)
                            .preferred_height(18.f))
                 .child(dsl::Row()
                            .fill_width()
                            .spacing(8.f)
                            .child(dsl::Button()
                                       .text(L"Primary")
                                       .hug_width()
                                       .fixed_height(32.f))
                            .child(dsl::Button()
                                       .text(L"Secondary")
                                       .variant(ButtonVariant::Secondary)
                                       .hug_width()
                                       .fixed_height(32.f))
                            .child(dsl::Checkbox().text(L"选项").checked(true)))
                 .child(dsl::TextField().placeholder(L"TextField 预览"))
                 .child(dsl::Slider().value(0.62f).fill_width())
                 .child(dsl::ProgressBar().value(0.45f).fill_width())
                 .child(std::move(status_b)))
      .Build();
}

}  // namespace

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int show) {
  Application::EnableDpiAwareness();
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

  Theme::RegisterBuiltInLight();
  Theme::RegisterBuiltInDark();
  Theme::SetActive("light");

  ThemeEditor editor;
  Window::WindowOptions opt;
  opt.caption = false;
  opt.resizable = true;
  opt.corner_radius = 8.f;
  opt.border_width = 1.f;
  opt.min_width = 420;
  opt.min_height = 520;
  if (!editor.window.Create(L"Theme Picker", 520, 720, opt)) {
    MessageBoxW(nullptr, L"Window init failed", L"theme_picker", MB_ICONERROR);
    CoUninitialize();
    return 1;
  }

  editor.base_ = MakeBuiltInLightTokens();
  editor.window.SetRoot(BuildUi(&editor));
  editor.WirePickers();
  editor.ApplyCustomTheme();

  ShowWindow(editor.window.hwnd(), show);
  UpdateWindow(editor.window.hwnd());
  const int code = Application::Run();
  CoUninitialize();
  return code;
}
