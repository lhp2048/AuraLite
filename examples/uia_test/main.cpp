// HWND-level UIA client tests (Inspect-style IUIAutomation).
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#include <oleauto.h>
#include <UIAutomation.h>

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

#include "auralite/ui/application.h"
#include "auralite/ui/button.h"
#include "auralite/ui/column.h"
#include "auralite/ui/tab.h"
#include "auralite/ui/window.h"

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

void Pump() {
  MSG msg;
  for (int i = 0; i < 16; ++i) {
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }
}

IUIAutomationElement* FindByName(IUIAutomation* uia, IUIAutomationElement* root,
                                 const wchar_t* name) {
  if (!uia || !root || !name) {
    return nullptr;
  }
  VARIANT v;
  VariantInit(&v);
  v.vt = VT_BSTR;
  v.bstrVal = SysAllocString(name);
  IUIAutomationCondition* cond = nullptr;
  const HRESULT hr = uia->CreatePropertyCondition(UIA_NamePropertyId, v, &cond);
  VariantClear(&v);
  if (FAILED(hr) || !cond) {
    return nullptr;
  }
  IUIAutomationElement* el = nullptr;
  root->FindFirst(TreeScope_Descendants, cond, &el);
  cond->Release();
  return el;
}

int ControlTypeOf(IUIAutomationElement* el) {
  if (!el) {
    return 0;
  }
  VARIANT v;
  VariantInit(&v);
  if (FAILED(el->GetCurrentPropertyValue(UIA_ControlTypePropertyId, &v))) {
    return 0;
  }
  const int id = (v.vt == VT_I4) ? v.lVal : 0;
  VariantClear(&v);
  return id;
}

}  // namespace

int main() {
  setvbuf(stdout, nullptr, _IONBF, 0);
  auralite::ui::Application::EnableDpiAwareness();
  if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
    std::puts("FAIL CoInitializeEx");
    return EXIT_FAILURE;
  }

  auralite::ui::Window window;
  auralite::ui::Window::WindowOptions opt;
  opt.quit_on_close = false;
  if (!window.Create(L"uia_test", 420, 320, opt)) {
    std::puts("FAIL Window::Create");
    CoUninitialize();
    return EXIT_FAILURE;
  }

  int clicks = 0;
  auto ok = std::make_unique<auralite::ui::Button>();
  ok->text(L"确定");
  ok->on_click([&] { ++clicks; });

  auto hidden = std::make_unique<auralite::ui::Button>();
  hidden->text(L"HiddenOnly");
  hidden->set_visible(false);

  auto menu = std::make_unique<auralite::ui::Button>();
  menu->text(L"Refresh");
  menu->set_acc_role(auralite::ui::AccRole::MenuItem);

  auto page0 = std::make_unique<auralite::ui::Column>();
  auto a = std::make_unique<auralite::ui::Button>();
  a->text(L"PageA");
  page0->AddChild(std::move(a));
  auto page1 = std::make_unique<auralite::ui::Column>();
  auto b = std::make_unique<auralite::ui::Button>();
  b->text(L"PageB");
  page1->AddChild(std::move(b));
  auto tab = std::make_unique<auralite::ui::Tab>();
  auralite::ui::Tab* tab_ptr = tab.get();
  tab->AddChild(std::move(page0));
  tab->AddChild(std::move(page1));
  tab->set_selected(0);

  auto root = std::make_unique<auralite::ui::Column>();
  root->AddChild(std::move(ok));
  root->AddChild(std::move(hidden));
  root->AddChild(std::move(menu));
  root->AddChild(std::move(tab));
  window.SetRoot(std::move(root));

  ShowWindow(window.hwnd(), SW_SHOWNA);
  UpdateWindow(window.hwnd());
  Pump();

  IUIAutomation* uia = nullptr;
  HRESULT hr = CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER,
                                IID_IUIAutomation, reinterpret_cast<void**>(&uia));
  Expect("CUIAutomation", SUCCEEDED(hr) && uia);
  if (!uia) {
    CoUninitialize();
    return EXIT_FAILURE;
  }

  IUIAutomationElement* root_el = nullptr;
  hr = uia->ElementFromHandle(window.hwnd(), &root_el);
  Expect("ElementFromHandle", SUCCEEDED(hr) && root_el);

  IUIAutomationElement* ok_el = FindByName(uia, root_el, L"确定");
  Expect("find 确定", ok_el != nullptr);
  Expect("确定 is Button", ControlTypeOf(ok_el) == UIA_ButtonControlTypeId);

  IUIAutomationElement* hidden_el = FindByName(uia, root_el, L"HiddenOnly");
  Expect("hidden not in tree", hidden_el == nullptr);

  IUIAutomationElement* menu_el = FindByName(uia, root_el, L"Refresh");
  Expect("find Refresh", menu_el != nullptr);
  Expect("Refresh is MenuItem", ControlTypeOf(menu_el) == UIA_MenuItemControlTypeId);

  IUIAutomationElement* page_a = FindByName(uia, root_el, L"PageA");
  IUIAutomationElement* page_b = FindByName(uia, root_el, L"PageB");
  Expect("tab page0 in tree", page_a != nullptr);
  Expect("tab page1 pruned", page_b == nullptr);

  if (ok_el) {
    IUIAutomationInvokePattern* invoke = nullptr;
    hr = ok_el->GetCurrentPatternAs(UIA_InvokePatternId,
                                    IID_IUIAutomationInvokePattern,
                                    reinterpret_cast<void**>(&invoke));
    Expect("Invoke pattern", SUCCEEDED(hr) && invoke);
    if (invoke) {
      Expect("Invoke HRESULT", SUCCEEDED(invoke->Invoke()));
      Pump();
      Expect("Invoke clicked", clicks == 1);
      invoke->Release();
    }
  }

  tab_ptr->set_selected(1);
  window.Invalidate();
  Pump();
  if (page_a) {
    page_a->Release();
    page_a = nullptr;
  }
  if (page_b) {
    page_b->Release();
    page_b = nullptr;
  }
  page_a = FindByName(uia, root_el, L"PageA");
  page_b = FindByName(uia, root_el, L"PageB");
  Expect("tab switch prunes A", page_a == nullptr);
  Expect("tab switch shows B", page_b != nullptr);

  if (ok_el) {
    ok_el->Release();
  }
  if (hidden_el) {
    hidden_el->Release();
  }
  if (menu_el) {
    menu_el->Release();
  }
  if (page_a) {
    page_a->Release();
  }
  if (page_b) {
    page_b->Release();
  }
  if (root_el) {
    root_el->Release();
  }
  uia->Release();
  CoUninitialize();

  if (g_failures) {
    std::printf("%d failed\n", g_failures);
    return EXIT_FAILURE;
  }
  std::printf("all ok\n");
  return EXIT_SUCCESS;
}
