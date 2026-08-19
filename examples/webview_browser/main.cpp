#include <windows.h>
#include <objbase.h>
#include <wrl/client.h>
#include <wrl/event.h>

#include <WebView2.h>

#include <algorithm>
#include <cwctype>
#include <string>

#include "auralite/ui.h"

namespace {

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;

constexpr wchar_t kHostClass[] = L"AuraLite.WebViewHost";
constexpr wchar_t kHome[] = L"https://www.bing.com/";

std::wstring Trim(std::wstring s) {
  auto not_space = [](wchar_t c) { return !std::iswspace(c); };
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), not_space));
  s.erase(std::find_if(s.rbegin(), s.rend(), not_space).base(), s.end());
  return s;
}

std::wstring NormalizeUrl(std::wstring raw) {
  raw = Trim(std::move(raw));
  if (raw.empty()) {
    return kHome;
  }
  if (raw.find(L"://") != std::wstring::npos) {
    return raw;
  }
  if (raw.rfind(L"about:", 0) == 0 || raw.rfind(L"data:", 0) == 0) {
    return raw;
  }
  return L"https://" + raw;
}

std::wstring UserDataFolder() {
  wchar_t appdata[MAX_PATH] = {};
  const DWORD n =
      GetEnvironmentVariableW(L"LOCALAPPDATA", appdata, MAX_PATH);
  if (n == 0 || n >= MAX_PATH) {
    return {};
  }
  std::wstring root = std::wstring(appdata) + L"\\AuraLite";
  std::wstring dir = root + L"\\webview_browser";
  CreateDirectoryW(root.c_str(), nullptr);
  CreateDirectoryW(dir.c_str(), nullptr);
  return dir;
}

struct BrowserApp {
  auralite::ui::Window window;
  auralite::ui::TextField* url = nullptr;
  auralite::ui::TitleBar* bar = nullptr;
  HWND host = nullptr;
  WNDPROC host_prev = nullptr;
  ComPtr<ICoreWebView2Controller> controller;
  ComPtr<ICoreWebView2> webview;
  bool closing = false;

  ~BrowserApp() { CloseWeb(); }

  void CloseWeb() {
    closing = true;
    if (controller) {
      controller->Close();
    }
    controller.Reset();
    webview.Reset();
    if (host && IsWindow(host)) {
      DestroyWindow(host);
    }
    host = nullptr;
    host_prev = nullptr;
  }

  void SyncBounds() {
    if (closing || !controller || !host || !IsWindow(host)) {
      return;
    }
    RECT rc = {};
    GetClientRect(host, &rc);
    controller->put_Bounds(rc);
  }

  void NavigateToField() {
    if (closing || !webview || !url) {
      return;
    }
    const std::wstring target = NormalizeUrl(url->text());
    url->text(target);
    window.Invalidate();
    webview->Navigate(target.c_str());
  }
};

LRESULT CALLBACK HostProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  auto* app =
      reinterpret_cast<BrowserApp*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
  const WNDPROC prev =
      (app && app->host_prev) ? app->host_prev : DefWindowProcW;
  if (msg == WM_SIZE && app) {
    app->SyncBounds();
  }
  if (msg == WM_NCDESTROY && app) {
    SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(prev));
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
    app->host_prev = nullptr;
    app->host = nullptr;
  }
  return CallWindowProcW(prev, hwnd, msg, wparam, lparam);
}

bool RegisterHostClass() {
  static bool done = false;
  if (done) {
    return true;
  }
  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = DefWindowProcW;
  wc.hInstance = GetModuleHandleW(nullptr);
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.lpszClassName = kHostClass;
  if (!RegisterClassExW(&wc)) {
    return GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
  }
  done = true;
  return true;
}

HWND CreateHostHwnd(HWND parent) {
  if (!RegisterHostClass() || !parent) {
    return nullptr;
  }
  return CreateWindowExW(0, kHostClass, L"", WS_CHILD | WS_CLIPSIBLINGS |
                                                 WS_CLIPCHILDREN,
                         0, 0, 0, 0, parent, nullptr,
                         GetModuleHandleW(nullptr), nullptr);
}

std::wstring UserDataOrTemp() {
  std::wstring dir = UserDataFolder();
  if (!dir.empty()) {
    return dir;
  }
  wchar_t tmp[MAX_PATH] = {};
  GetTempPathW(MAX_PATH, tmp);
  return std::wstring(tmp) + L"AuraLiteWebView";
}

HRESULT StartWebView(BrowserApp* app) {
  if (!app || !app->host) {
    return E_POINTER;
  }
  const std::wstring data = UserDataOrTemp();
  return CreateCoreWebView2EnvironmentWithOptions(
      nullptr, data.c_str(), nullptr,
      Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
          [app](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {
            if (app->closing || FAILED(result) || !env) {
              if (!app->closing) {
                MessageBoxW(app->window.hwnd(),
                            L"WebView2 环境创建失败。请安装 Evergreen Runtime：\n"
                            L"https://go.microsoft.com/fwlink/p/?LinkId=2124703",
                            L"webview_browser", MB_ICONERROR);
              }
              return S_OK;
            }
            return env->CreateCoreWebView2Controller(
                app->host,
                Callback<
                    ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [app](HRESULT result,
                          ICoreWebView2Controller* controller) -> HRESULT {
                      if (app->closing || FAILED(result) || !controller) {
                        return S_OK;
                      }
                      app->controller = controller;
                      controller->get_CoreWebView2(&app->webview);
                      if (!app->webview) {
                        return S_OK;
                      }
                      controller->put_IsVisible(TRUE);
                      app->SyncBounds();

                      app->webview->add_SourceChanged(
                          Callback<ICoreWebView2SourceChangedEventHandler>(
                              [app](ICoreWebView2* sender,
                                    ICoreWebView2SourceChangedEventArgs*)
                                  -> HRESULT {
                                if (app->closing || !sender || !app->url) {
                                  return S_OK;
                                }
                                wchar_t* uri = nullptr;
                                if (SUCCEEDED(sender->get_Source(&uri)) &&
                                    uri) {
                                  app->url->text(uri);
                                  CoTaskMemFree(uri);
                                  app->window.Invalidate();
                                }
                                return S_OK;
                              })
                              .Get(),
                          nullptr);

                      app->webview->add_DocumentTitleChanged(
                          Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
                              [app](ICoreWebView2* sender, IUnknown*)
                                  -> HRESULT {
                                if (app->closing || !sender || !app->bar) {
                                  return S_OK;
                                }
                                wchar_t* title = nullptr;
                                if (SUCCEEDED(
                                        sender->get_DocumentTitle(&title)) &&
                                    title) {
                                  app->bar->title(title);
                                  CoTaskMemFree(title);
                                  app->window.Invalidate();
                                }
                                return S_OK;
                              })
                              .Get(),
                          nullptr);

                      app->NavigateToField();
                      return S_OK;
                    })
                    .Get());
          })
          .Get());
}

}  // namespace

int APIENTRY wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int show) {
  auralite::ui::Application::EnableDpiAwareness();
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

  BrowserApp app;
  auralite::ui::Window::WindowOptions opt;
  opt.caption = false;
  opt.resizable = true;
  opt.corner_radius = 8.f;
  opt.border_width = 1.f;
  opt.quit_on_close = true;
  opt.min_width = 480;
  opt.min_height = 320;
  if (!app.window.Create(L"AuraLite Browser", 960, 640, opt)) {
    MessageBoxW(nullptr, L"Window / Canvas init failed", L"webview_browser",
                MB_ICONERROR);
    CoUninitialize();
    return 1;
  }

  using namespace auralite::ui::dsl;

  auto url_b = TextField();
  url_b.text(kHome).placeholder(L"输入网址，回车打开").font_size(13.f);
  app.url = url_b.get();
  auto go_b = Button();
  go_b.text(L"转到").hug_width().fixed_height(32.f).is_default(true);
  auralite::ui::Button* go = go_b.get();
  auto host_b = NativeHost();
  host_b.name("webview").fill_width().fill_height();
  auralite::ui::NativeHost* native = host_b.get();
  auto bar_b = TitleBar();
  bar_b.title(L"浏览器");
  app.bar = bar_b.get();

  auto go_nav = [&app] { app.NavigateToField(); };
  app.url->on_submit(go_nav);
  go->on_click(go_nav);

  app.window.SetRoot(Column()
                         .fill_width()
                         .fill_height()
                         .padding(1.f, 0.f, 1.f, 1.f)
                         .child(std::move(bar_b))
                         .child(Row()
                                    .fill_width()
                                    .padding(8.f, 6.f, 8.f, 6.f)
                                    .spacing(8.f)
                                    .v_align(auralite::ui::Align::Center)
                                    .child(std::move(url_b))
                                    .child(std::move(go_b)))
                         .child(std::move(host_b))
                         .Build());

  app.host = CreateHostHwnd(app.window.hwnd());
  if (!app.host || !native) {
    MessageBoxW(app.window.hwnd(), L"WebView 宿主 HWND 创建失败",
                L"webview_browser", MB_ICONERROR);
    CoUninitialize();
    return 1;
  }
  native->AttachBorrowed(app.host);
  SetWindowLongPtrW(app.host, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&app));
  app.host_prev = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(
      app.host, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HostProc)));

  const HRESULT hr = StartWebView(&app);
  if (FAILED(hr)) {
    MessageBoxW(app.window.hwnd(),
                L"WebView2 Loader 调用失败。请安装 Evergreen Runtime。",
                L"webview_browser", MB_ICONERROR);
  }

  ShowWindow(app.window.hwnd(), show);
  UpdateWindow(app.window.hwnd());
  const int code = auralite::ui::Application::Run();
  app.CloseWeb();
  CoUninitialize();
  return code;
}
