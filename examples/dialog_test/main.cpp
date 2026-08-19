#include <cstdio>
#include <windows.h>

#include "mx/async/task_lambda.h"
#include "mx/ui/button.h"
#include "mx/ui/column.h"
#include "mx/ui/native_host.h"
#include "mx/ui/text_area.h"
#include "mx/ui/text_field.h"
#include "mx/ui/title_bar.h"
#include "mx/ui/window.h"
#include "base/at_exit.h"
#include "message_framework/message_loop.h"

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

}  // namespace

int main() {
  using mx::ui::Window;

  {
    Window::WindowOptions o;
    o.caption = true;
    o.corner_radius = -3.f;
    o.border_width = -1.f;
    o.Normalize();
    Expect(o.corner_radius == 0.f, "Normalize caption radius");
    Expect(o.border_width == 0.f, "Normalize caption border");
    Window::WindowOptions d = Window::WindowOptions::Dialog(nullptr, -4.f);
    Expect(!d.caption, "Dialog no caption");
    Expect(d.corner_radius == 0.f, "Dialog negative radius");
    Expect(d.border_width == 1.f, "Dialog border");
    Expect(!d.resizable, "Dialog not resizable");
  }

  {
    Window w;
    Expect(w.RunModal() == IDABORT, "RunModal no hwnd");
    w.EndModal(IDOK);
    Expect(w.modal_result() == IDOK, "EndModal without Run stores result");
  }

  {
    Window app;
    Expect(app.Create(L"app", 200, 120), "Create captioned");
    HWND hwnd = app.hwnd();
    Expect(hwnd != nullptr, "app hwnd");
    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    Expect((style & WS_CAPTION) != 0, "app has caption");
    DestroyWindow(hwnd);
  }

  {
    Window app;
    Window::WindowOptions o;
    o.caption = true;
    o.corner_radius = 8.f;
    o.border_width = 1.f;
    Expect(app.Create(L"app", 200, 120, o), "Create captioned with radius");
    HWND hwnd = app.hwnd();
    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    Expect((style & WS_CAPTION) != 0, "captioned still has caption");
    Expect(app.options().caption, "options caption true");
    Expect(app.options().corner_radius == 0.f, "options stores normalized radius");
    Expect(app.options().border_width == 0.f, "options stores normalized border");
    Expect(app.RunModal() == IDABORT, "captioned RunModal abort");
    HRGN probe = CreateRectRgn(0, 0, 0, 0);
    const int rgn_type = GetWindowRgn(hwnd, probe);
    Expect(rgn_type == ERROR, "captioned window has no library rgn");
    DeleteObject(probe);
    DestroyWindow(hwnd);
  }

  {
    Window w;
    Window::WindowOptions o;
    o.caption = false;
    o.quit_on_close = false;
    o.topmost = false;
    o.corner_radius = 8.f;
    o.border_width = 1.f;
    Expect(w.Create(L"modeless", 200, 120, o), "Create modeless");
    HWND hwnd = w.hwnd();
    Expect(hwnd != nullptr, "modeless hwnd");
    Expect(w.is_dialog(), "modeless custom chrome");
    Expect(!w.options().caption, "modeless options no caption");
    LONG modeless_style = GetWindowLongW(hwnd, GWL_STYLE);
    LONG modeless_ex = GetWindowLongW(hwnd, GWL_EXSTYLE);
    Expect((modeless_style & WS_MINIMIZEBOX) != 0, "modeless WS_MINIMIZEBOX");
    Expect((modeless_style & WS_MAXIMIZEBOX) != 0, "modeless WS_MAXIMIZEBOX");
    Expect((modeless_ex & WS_EX_APPWINDOW) != 0, "modeless WS_EX_APPWINDOW");
    Expect(GetWindow(hwnd, GW_OWNER) == nullptr, "modeless unowned");
    Expect(w.options().corner_radius == 8.f, "modeless keeps radius");
    Expect(w.options().quit_on_close == false, "modeless quit_on_close");
    SendMessageW(hwnd, WM_KEYDOWN, VK_ESCAPE, 0);
    Expect(IsWindow(hwnd) && w.hwnd() == hwnd, "modeless Esc does not destroy");
    w.Close();
    Expect(!IsWindow(hwnd), "Close destroys modeless");
    Expect(w.hwnd() == nullptr, "Close clears hwnd");
  }

  {
    Window owner;
    Expect(owner.Create(L"owner-app", 320, 240), "owner for frameless app");
    Window w;
    Window::WindowOptions o;
    o.caption = false;
    o.quit_on_close = false;
    o.resizable = true;
    o.owner = owner.hwnd();
    o.center_on_owner = true;
    Expect(w.Create(L"frameless-app", 200, 120, o), "Create frameless app");
    Expect(GetWindow(w.hwnd(), GW_OWNER) == nullptr,
           "frameless app ignores owner HWND");
    ShowWindow(w.hwnd(), SW_SHOW);
    w.Minimize();
    Expect(IsIconic(w.hwnd()) != FALSE, "Minimize makes iconic");
    w.Close();
    DestroyWindow(owner.hwnd());
  }

  {
    using mx::ui::Column;
    using mx::ui::TitleBar;
    Window w;
    Window::WindowOptions o;
    o.caption = false;
    o.quit_on_close = false;
    o.resizable = true;
    Expect(w.Create(L"dblclk", 360, 240, o), "Create dblclk");
    auto root = std::make_unique<Column>();
    root->fill_width().fill_height();
    auto bar = std::make_unique<TitleBar>();
    bar->title(L"t");
    root->AddChild(std::move(bar));
    w.SetRoot(std::move(root));
    w.root()->Layout(mx::ui::RectF{0.f, 0.f, 360.f, 240.f});
    const float dpi = w.dpi();
    const int x = static_cast<int>(mx::PxFromDip(40.f, dpi));
    const int y = static_cast<int>(mx::PxFromDip(18.f, dpi));
    SendMessageW(w.hwnd(), WM_LBUTTONDBLCLK, MK_LBUTTON, MAKELPARAM(x, y));
    Expect(w.is_maximized(), "titlebar dblclk maximizes");
    w.Close();
  }

  {
    using mx::ui::Column;
    using mx::ui::TitleBar;
    Window w;
    Window::WindowOptions o;
    o.caption = false;
    o.quit_on_close = false;
    o.resizable = true;
    Expect(w.Create(L"minbtn", 360, 240, o), "Create minbtn");
    auto root = std::make_unique<Column>();
    root->fill_width().fill_height();
    auto bar = std::make_unique<TitleBar>();
    bar->title(L"t");
    root->AddChild(std::move(bar));
    w.SetRoot(std::move(root));
    w.root()->Layout(mx::ui::RectF{0.f, 0.f, 360.f, 240.f});
    mx::ui::Node* min = w.root()->FindByName("minimize");
    Expect(min != nullptr, "min slot exists");
    ShowWindow(w.hwnd(), SW_SHOW);
    if (min) {
      const mx::ui::RectF b = min->bounds();
      const float dpi = w.dpi();
      const int x =
          static_cast<int>(mx::PxFromDip(b.x + b.w * 0.5f, dpi));
      const int y =
          static_cast<int>(mx::PxFromDip(b.y + b.h * 0.5f, dpi));
      SendMessageW(w.hwnd(), WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(x, y));
      SendMessageW(w.hwnd(), WM_LBUTTONUP, 0, MAKELPARAM(x, y));
    }
    Expect(IsIconic(w.hwnd()) != FALSE, "min button click iconic");
    w.Close();
  }

  {
    using mx::ui::Column;
    using mx::ui::TitleBar;
    Window dlg;
    Expect(dlg.Create(L"dlg-dbl", 200, 120, Window::WindowOptions::Dialog()),
           "Create dialog dblclk");
    auto root = std::make_unique<Column>();
    auto bar = std::make_unique<TitleBar>();
    bar->title(L"d").minimize(false).maximize(false);
    root->AddChild(std::move(bar));
    dlg.SetRoot(std::move(root));
    dlg.root()->Layout(mx::ui::RectF{0.f, 0.f, 200.f, 120.f});
    const float dpi = dlg.dpi();
    const int x = static_cast<int>(mx::PxFromDip(40.f, dpi));
    const int y = static_cast<int>(mx::PxFromDip(18.f, dpi));
    SendMessageW(dlg.hwnd(), WM_LBUTTONDBLCLK, MK_LBUTTON, MAKELPARAM(x, y));
    Expect(!dlg.is_maximized(), "dialog dblclk does not maximize");
    dlg.Close();
  }

  {
    Window dlg;
    Expect(dlg.Create(L"dlg", 200, 120, Window::WindowOptions::Dialog()),
           "Create with Dialog options");
    HWND hwnd = dlg.hwnd();
    Expect(hwnd != nullptr, "dlg hwnd");
    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    Expect((style & WS_CAPTION) == 0, "Dialog options no caption");
    Expect((style & WS_POPUP) != 0, "Dialog options WS_POPUP");
    Expect((style & WS_MINIMIZEBOX) == 0, "Dialog options no WS_MINIMIZEBOX");
    Expect(dlg.is_dialog(), "Dialog options is_dialog");
    HRGN probe = CreateRectRgn(0, 0, 0, 0);
    const int rgn_type = GetWindowRgn(hwnd, probe);
    Expect(rgn_type == COMPLEXREGION || rgn_type == SIMPLEREGION,
           "Dialog options rounded rgn");
    DeleteObject(probe);
    DestroyWindow(hwnd);
  }

  {
    Window dlg;
    Expect(dlg.Create(L"square", 200, 120,
                      Window::WindowOptions::Dialog(nullptr, 0.f)),
           "Create square Dialog");
    HWND hwnd = dlg.hwnd();
    Expect(dlg.is_dialog(), "square is_dialog");
    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    Expect((style & WS_CAPTION) == 0, "square no caption");
    HRGN probe = CreateRectRgn(0, 0, 0, 0);
    const int rgn_type = GetWindowRgn(hwnd, probe);
    Expect(rgn_type == ERROR, "square Dialog has no rgn");
    DeleteObject(probe);
    DestroyWindow(hwnd);
  }

  base::AtExitManager at_exit;
  MessageLoopForUI loop;
  Window dlg;
  Window owner;
  Expect(owner.Create(L"owner", 320, 240), "owner Create");
  EnableWindow(owner.hwnd(), TRUE);
  Expect(dlg.Create(L"dlg", 200, 120, Window::WindowOptions::Dialog(owner.hwnd())),
         "dlg create");
  mx::async::PostFn([&] {
    Expect(!IsWindowEnabled(owner.hwnd()), "owner disabled in modal");
    dlg.EndModal(IDOK);
  });
  Expect(dlg.RunModal() == IDOK, "RunModal returns IDOK");
  Expect(IsWindowEnabled(owner.hwnd()), "owner re-enabled");
  DestroyWindow(owner.hwnd());

  {
    using mx::ui::Button;
    using mx::ui::Column;
    using mx::ui::TextArea;
    using mx::ui::TextField;

    Window w;
    Expect(w.Create(L"def", 280, 160, Window::WindowOptions::Dialog()),
           "default-btn Create");
    int clicks = 0;
    auto col = std::make_unique<Column>();
    auto field = std::make_unique<TextField>();
    TextField* field_ptr = field.get();
    auto ok = std::make_unique<Button>();
    ok->text(L"OK").is_default(true).on_click([&] { ++clicks; });
    col->AddChild(std::move(field));
    col->AddChild(std::move(ok));
    w.SetRoot(std::move(col));
    w.SetFocusNode(field_ptr);
    SendMessageW(w.hwnd(), WM_KEYDOWN, VK_RETURN, 0);
    Expect(clicks == 1, "Enter on TextField clicks default button");
    w.Close();
  }

  {
    using mx::ui::Button;
    using mx::ui::Column;

    Window w;
    Expect(w.Create(L"def2", 280, 160, Window::WindowOptions::Dialog()),
           "focused-btn Create");
    int def_clicks = 0;
    int other_clicks = 0;
    auto col = std::make_unique<Column>();
    auto other = std::make_unique<Button>();
    Button* other_ptr = other.get();
    other->text(L"Other").on_click([&] { ++other_clicks; });
    auto ok = std::make_unique<Button>();
    ok->text(L"OK").is_default(true).on_click([&] { ++def_clicks; });
    col->AddChild(std::move(other));
    col->AddChild(std::move(ok));
    w.SetRoot(std::move(col));
    w.SetFocusNode(other_ptr);
    SendMessageW(w.hwnd(), WM_KEYDOWN, VK_RETURN, 0);
    Expect(other_clicks == 1 && def_clicks == 0,
           "Enter on focused Button clicks it, not default");
    w.Close();
  }

  {
    using mx::ui::Button;
    using mx::ui::Column;
    using mx::ui::TextArea;

    Window w;
    Expect(w.Create(L"def3", 280, 160, Window::WindowOptions::Dialog()),
           "textarea Create");
    int clicks = 0;
    auto col = std::make_unique<Column>();
    auto area = std::make_unique<TextArea>();
    TextArea* area_ptr = area.get();
    auto ok = std::make_unique<Button>();
    ok->text(L"OK").is_default(true).on_click([&] { ++clicks; });
    col->AddChild(std::move(area));
    col->AddChild(std::move(ok));
    w.SetRoot(std::move(col));
    w.SetFocusNode(area_ptr);
    SendMessageW(w.hwnd(), WM_KEYDOWN, VK_RETURN, 0);
    Expect(clicks == 0, "Enter on TextArea does not click default");
    w.Close();
  }

  {
    using mx::ui::Button;
    using mx::ui::Column;
    using mx::ui::TextField;

    Window w;
    Expect(w.Create(L"def4", 280, 160, Window::WindowOptions::Dialog()),
           "disabled default Create");
    int clicks = 0;
    auto col = std::make_unique<Column>();
    auto field = std::make_unique<TextField>();
    TextField* field_ptr = field.get();
    auto ok = std::make_unique<Button>();
    ok->text(L"OK").is_default(true).set_enabled(false).on_click(
        [&] { ++clicks; });
    col->AddChild(std::move(field));
    col->AddChild(std::move(ok));
    w.SetRoot(std::move(col));
    w.SetFocusNode(field_ptr);
    SendMessageW(w.hwnd(), WM_KEYDOWN, VK_RETURN, 0);
    Expect(clicks == 0, "disabled default is not clicked");
    w.Close();
  }

  {
    using mx::ui::KeyChord;
    using mx::ui::KeyEvent;
    using mx::ui::ParseKeyChord;

    mx::ui::KeyChord chord;
    Expect(ParseKeyChord("Ctrl+S", &chord) && chord.vk == 'S' && chord.ctrl &&
               !chord.alt && !chord.shift,
           "Parse Ctrl+S");
    Expect(ParseKeyChord("f1", &chord) && chord.vk == VK_F1 && !chord.ctrl,
           "Parse F1");
    Expect(ParseKeyChord("Esc", &chord) && chord.vk == VK_ESCAPE,
           "Parse Esc");
    Expect(!ParseKeyChord("S", &chord), "bare letter is not a shortcut");
    Expect(!ParseKeyChord("Ctrl+", &chord), "Ctrl+ alone invalid");
  }

  {
    using mx::ui::Button;
    using mx::ui::Column;
    using mx::ui::KeyEvent;
    using mx::ui::TextField;

    Window w;
    Expect(w.Create(L"accel", 280, 160, Window::WindowOptions::Dialog()),
           "accel Create");
    int win_hits = 0;
    int btn_hits = 0;
    Expect(w.AddAccelerator("Ctrl+S", [&] { ++win_hits; }), "Add Ctrl+S");
    Expect(!w.AddAccelerator("S", [&] {}), "reject bare S");

    auto col = std::make_unique<Column>();
    auto field = std::make_unique<TextField>();
    TextField* field_ptr = field.get();
    auto help = std::make_unique<Button>();
    help->text(L"Help").accelerator("F1").on_click([&] { ++btn_hits; });
    col->AddChild(std::move(field));
    col->AddChild(std::move(help));
    w.SetRoot(std::move(col));
    w.SetFocusNode(field_ptr);

    KeyEvent save;
    save.vk = 'S';
    save.down = true;
    save.ctrl = true;
    Expect(w.HandleKey(save), "Ctrl+S handled");
    Expect(win_hits == 1, "Ctrl+S fires window accelerator");

    KeyEvent f1;
    f1.vk = VK_F1;
    f1.down = true;
    Expect(w.HandleKey(f1), "F1 handled");
    Expect(btn_hits == 1, "F1 fires button accelerator");

    KeyEvent letter;
    letter.vk = 'A';
    letter.down = true;
    Expect(!w.HandleKey(letter), "plain A is not an accelerator");
    Expect(win_hits == 1 && btn_hits == 1, "plain A does not fire shortcuts");
    w.Close();
  }

  {
    using mx::ui::Column;
    using mx::ui::NativeHost;
    Window w;
    Window::WindowOptions o;
    o.quit_on_close = false;
    Expect(w.Create(L"native-host", 240, 160, o), "Create native-host");
    HWND guest = CreateWindowExW(0, L"STATIC", L"guest", WS_POPUP, 0, 0, 40, 20,
                                 nullptr, nullptr, GetModuleHandleW(nullptr),
                                 nullptr);
    Expect(guest != nullptr, "guest hwnd");
    auto root = std::make_unique<Column>();
    root->fill_width().fill_height();
    auto host = std::make_unique<NativeHost>();
    NativeHost* ph = host.get();
    host->fixed_height(80.f);
    root->AddChild(std::move(host));
    w.SetRoot(std::move(root));
    w.root()->Layout(mx::ui::RectF{0.f, 0.f, 240.f, 160.f});
    ph->AttachBorrowed(guest);
    Expect(ph->hwnd() == guest, "Attach stores hwnd");
    Expect(!ph->owns_hwnd(), "borrowed does not own");
    Expect(GetParent(guest) == w.hwnd(), "guest parented to window");
    RECT gr = {};
    GetClientRect(guest, &gr);
    Expect(gr.right > 0 && gr.bottom > 0, "guest sized");
    ph->Detach();
    Expect(ph->hwnd() == nullptr, "Detach clears hwnd");
    Expect(IsWindow(guest) != FALSE, "borrowed Detach does not destroy");
    Expect(GetParent(guest) != w.hwnd(), "Detach unparents");
    ph->AttachBorrowed(guest);
    w.Close();
    Expect(IsWindow(guest) != FALSE, "borrowed Close does not destroy");
    DestroyWindow(guest);
  }

  {
    using mx::ui::Column;
    using mx::ui::NativeHost;
    Window w;
    Window::WindowOptions o;
    o.quit_on_close = false;
    Expect(w.Create(L"native-owned", 240, 160, o), "Create native-owned");
    HWND guest = CreateWindowExW(0, L"STATIC", L"owned", WS_POPUP, 0, 0, 40, 20,
                                 nullptr, nullptr, GetModuleHandleW(nullptr),
                                 nullptr);
    Expect(guest != nullptr, "owned guest hwnd");
    auto root = std::make_unique<Column>();
    auto host = std::make_unique<NativeHost>();
    NativeHost* ph = host.get();
    host->fixed_height(80.f);
    root->AddChild(std::move(host));
    w.SetRoot(std::move(root));
    w.root()->Layout(mx::ui::RectF{0.f, 0.f, 240.f, 160.f});
    ph->Attach(guest);
    Expect(ph->owns_hwnd(), "Attach owns by default");
    w.Close();
    Expect(IsWindow(guest) == FALSE, "owned Close destroys guest");
  }

  {
    using mx::ui::Column;
    using mx::ui::NativeHost;
    Window w;
    Window::WindowOptions o;
    o.quit_on_close = false;
    Expect(w.Create(L"native-release", 240, 160, o), "Create native-release");
    HWND guest = CreateWindowExW(0, L"STATIC", L"rel", WS_POPUP, 0, 0, 40, 20,
                                 nullptr, nullptr, GetModuleHandleW(nullptr),
                                 nullptr);
    auto root = std::make_unique<Column>();
    auto host = std::make_unique<NativeHost>();
    NativeHost* ph = host.get();
    host->fixed_height(80.f);
    root->AddChild(std::move(host));
    w.SetRoot(std::move(root));
    w.root()->Layout(mx::ui::RectF{0.f, 0.f, 240.f, 160.f});
    ph->Attach(guest);
    HWND released = ph->Release();
    Expect(released == guest, "Release returns hwnd");
    Expect(!ph->owns_hwnd(), "Release drops ownership");
    Expect(IsWindow(guest) != FALSE, "Release does not destroy");
    w.Close();
    Expect(IsWindow(guest) != FALSE, "released survives Close");
    DestroyWindow(guest);
  }

  if (g_failures) {
    std::printf("%d failed\n", g_failures);
    return 1;
  }
  std::printf("all passed\n");
  return 0;
}
