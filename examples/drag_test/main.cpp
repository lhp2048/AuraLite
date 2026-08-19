#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>

#include "mx/canvas.h"
#include "mx/ui/button.h"
#include "mx/ui/column.h"
#include "mx/ui/factory.h"
#include "mx/ui/label.h"
#include "mx/ui/window.h"
#include "mx/ui/yaml_loader.h"

namespace {

int g_failures = 0;

void Expect(const char* name, bool cond) {
  if (!cond) {
    std::printf("FAIL %s\n", name);
    ++g_failures;
  } else {
    std::printf("ok   %s\n", name);
  }
}

LPARAM PackDip(float dip_x, float dip_y, float dpi) {
  const int x = static_cast<int>(mx::PxFromDip(dip_x, dpi));
  const int y = static_cast<int>(mx::PxFromDip(dip_y, dpi));
  return MAKELPARAM(x, y);
}

HDROP MakeHDrop(const std::vector<std::wstring>& paths, POINT pt) {
  size_t chars = 1;
  for (const auto& p : paths) {
    chars += p.size() + 1;
  }
  const size_t bytes = sizeof(DROPFILES) + chars * sizeof(wchar_t);
  HGLOBAL mem = GlobalAlloc(GHND, static_cast<SIZE_T>(bytes));
  if (!mem) {
    return nullptr;
  }
  auto* df = static_cast<DROPFILES*>(GlobalLock(mem));
  df->pFiles = sizeof(DROPFILES);
  df->pt = pt;
  df->fNC = FALSE;
  df->fWide = TRUE;
  wchar_t* dest = reinterpret_cast<wchar_t*>(reinterpret_cast<BYTE*>(df) +
                                             sizeof(DROPFILES));
  for (const auto& p : paths) {
    std::memcpy(dest, p.c_str(), (p.size() + 1) * sizeof(wchar_t));
    dest += p.size() + 1;
  }
  *dest = L'\0';
  GlobalUnlock(mem);
  return reinterpret_cast<HDROP>(mem);
}

}  // namespace

int main() {
  using namespace mx::ui;

  {
    Label n;
    Expect("default not draggable", !n.draggable());
    Expect("default drag_data empty", n.drag_data().empty());
    Expect("default not drop target", !n.drop_target());
    n.draggable(true).drag_data(L"chip").drop_target(true);
    Expect("draggable on", n.draggable());
    Expect("drag_data", n.drag_data() == L"chip");
    Expect("drop_target on", n.drop_target());
    bool dropped = false;
    n.on_drop([&](const DragEvent&) { dropped = true; });
    Expect("on_drop implies drop_target", n.drop_target());
  }

  {
    ViewFactory factory;
    auto n = LoadYamlString(
        "Label:\n"
        "  text: x\n"
        "  draggable: true\n"
        "  drag_data: chip\n"
        "  drop_target: true\n",
        factory, {});
    auto* lab = dynamic_cast<Label*>(n.get());
    Expect("yaml label", lab != nullptr);
    Expect("yaml draggable", lab && lab->draggable());
    Expect("yaml drag_data", lab && lab->drag_data() == L"chip");
    Expect("yaml drop_target", lab && lab->drop_target());
  }

  {
    Window w;
    Window::WindowOptions opt;
    opt.quit_on_close = false;
    Expect("create", w.Create(L"drag", 200, 120, opt));
    HWND hwnd = w.hwnd();
    Expect("hwnd", hwnd != nullptr);
    const float dpi = w.dpi();

    auto root = std::make_unique<Column>();
    root->spacing(0.f).fill_width().fill_height();
    auto src = std::make_unique<Label>();
    src->text(L"src").fixed_height(40.f).fill_width();
    src->draggable(true).drag_data(L"chip");
    auto dst = std::make_unique<Label>();
    dst->text(L"dst").fixed_height(40.f).fill_width();
    std::wstring got;
    Node* got_src = nullptr;
    dst->on_drop([&](const DragEvent& e) {
      got = e.data;
      got_src = e.source;
    });
    Label* psrc = src.get();
    root->AddChild(std::move(src));
    root->AddChild(std::move(dst));
    w.SetRoot(std::move(root));
    w.root()->Layout(RectF{0.f, 0.f, 200.f, 80.f});

    SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, PackDip(10.f, 20.f, dpi));
    SendMessageW(hwnd, WM_MOUSEMOVE, MK_LBUTTON, PackDip(12.f, 21.f, dpi));
    Expect("below slop not dragging", !w.is_dragging());
    SendMessageW(hwnd, WM_MOUSEMOVE, MK_LBUTTON, PackDip(10.f, 50.f, dpi));
    Expect("past slop dragging", w.is_dragging());
    SendMessageW(hwnd, WM_LBUTTONUP, 0, PackDip(10.f, 60.f, dpi));
    Expect("drop data", got == L"chip");
    Expect("drop source", got_src == psrc);
    Expect("not dragging after up", !w.is_dragging());
    DestroyWindow(hwnd);
  }

  {
    Window w;
    Window::WindowOptions opt;
    opt.quit_on_close = false;
    Expect("create click", w.Create(L"click", 200, 80, opt));
    HWND hwnd = w.hwnd();
    const float dpi = w.dpi();
    int clicks = 0;
    auto btn = std::make_unique<Button>();
    btn->text(L"b").fixed_height(40.f).fill_width();
    btn->draggable(true).drag_data(L"b");
    btn->on_click([&] { ++clicks; });
    auto root = std::make_unique<Column>();
    root->AddChild(std::move(btn));
    w.SetRoot(std::move(root));
    w.root()->Layout(RectF{0.f, 0.f, 200.f, 40.f});

    SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, PackDip(10.f, 20.f, dpi));
    SendMessageW(hwnd, WM_LBUTTONUP, 0, PackDip(10.f, 20.f, dpi));
    Expect("click without drag", clicks == 1);

    SendMessageW(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, PackDip(10.f, 20.f, dpi));
    SendMessageW(hwnd, WM_MOUSEMOVE, MK_LBUTTON, PackDip(10.f, 50.f, dpi));
    SendMessageW(hwnd, WM_LBUTTONUP, 0, PackDip(10.f, 50.f, dpi));
    Expect("drag suppresses click", clicks == 1);
    DestroyWindow(hwnd);
  }

  {
    Window w;
    Expect("default no files", !w.accept_files());
    Window::WindowOptions opt;
    opt.quit_on_close = false;
    Expect("create files", w.Create(L"files", 200, 120, opt));
    HWND hwnd = w.hwnd();
    std::vector<std::wstring> got;
    int calls = 0;
    w.set_on_files_dropped([&](const FileDropEvent& e) {
      ++calls;
      got = e.paths;
    });
    HDROP ignored = MakeHDrop({L"C:\\ignored.txt"}, POINT{8, 8});
    SendMessageW(hwnd, WM_DROPFILES, reinterpret_cast<WPARAM>(ignored), 0);
    Expect("closed until accept_files", calls == 0);

    w.set_accept_files(true);
    Expect("accept_files on", w.accept_files());
    HDROP drop = MakeHDrop({L"C:\\a.txt", L"D:\\b.png"}, POINT{8, 8});
    SendMessageW(hwnd, WM_DROPFILES, reinterpret_cast<WPARAM>(drop), 0);
    Expect("files callback", calls == 1);
    Expect("two paths", got.size() == 2);
    Expect("path0", !got.empty() && got[0] == L"C:\\a.txt");
    Expect("path1", got.size() == 2 && got[1] == L"D:\\b.png");

    w.set_accept_files(false);
    HDROP again = MakeHDrop({L"C:\\c.txt"}, POINT{8, 8});
    SendMessageW(hwnd, WM_DROPFILES, reinterpret_cast<WPARAM>(again), 0);
    Expect("off ignores drop", calls == 1);
    DestroyWindow(hwnd);
  }

  if (g_failures) {
    std::printf("%d failed\n", g_failures);
    return 1;
  }
  std::printf("all passed\n");
  return 0;
}
