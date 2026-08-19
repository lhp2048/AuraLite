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

#include "mx/ui/application.h"
#include "mx/ui/button.h"
#include "mx/ui/checkbox.h"
#include "mx/ui/column.h"
#include "mx/ui/combo.h"
#include "mx/ui/progress_bar.h"
#include "mx/ui/slider.h"
#include "mx/ui/tab.h"
#include "mx/ui/window.h"

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

class PropSink : public IUIAutomationPropertyChangedEventHandler {
 public:
  ULONG STDMETHODCALLTYPE AddRef() override {
    return static_cast<ULONG>(InterlockedIncrement(&ref_));
  }
  ULONG STDMETHODCALLTYPE Release() override {
    const ULONG n = static_cast<ULONG>(InterlockedDecrement(&ref_));
    if (n == 0) {
      delete this;
    }
    return n;
  }
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
    if (!ppv) {
      return E_POINTER;
    }
    if (riid == IID_IUnknown ||
        riid == IID_IUIAutomationPropertyChangedEventHandler) {
      *ppv = static_cast<IUIAutomationPropertyChangedEventHandler*>(this);
      AddRef();
      return S_OK;
    }
    *ppv = nullptr;
    return E_NOINTERFACE;
  }
  HRESULT STDMETHODCALLTYPE HandlePropertyChangedEvent(
      IUIAutomationElement* /*sender*/, PROPERTYID /*propertyId*/,
      VARIANT /*newValue*/) override {
    InterlockedIncrement(&count_);
    return S_OK;
  }
  int count() const { return static_cast<int>(count_); }

 private:
  LONG ref_ = 1;
  LONG count_ = 0;
};

}  // namespace

int main() {
  setvbuf(stdout, nullptr, _IONBF, 0);
  mx::ui::Application::EnableDpiAwareness();
  if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED))) {
    std::puts("FAIL CoInitializeEx");
    return EXIT_FAILURE;
  }

  mx::ui::Window window;
  mx::ui::Window::WindowOptions opt;
  opt.quit_on_close = false;
  if (!window.Create(L"uia_test", 480, 420, opt)) {
    std::puts("FAIL Window::Create");
    CoUninitialize();
    return EXIT_FAILURE;
  }

  int clicks = 0;
  auto ok = std::make_unique<mx::ui::Button>();
  ok->text(L"确定");
  ok->on_click([&] { ++clicks; });

  auto hidden = std::make_unique<mx::ui::Button>();
  hidden->text(L"HiddenOnly");
  hidden->set_visible(false);

  auto menu = std::make_unique<mx::ui::Button>();
  menu->text(L"Refresh");
  menu->set_acc_role(mx::ui::AccRole::MenuItem);

  auto page0 = std::make_unique<mx::ui::Column>();
  auto a = std::make_unique<mx::ui::Button>();
  a->text(L"PageA");
  page0->AddChild(std::move(a));
  auto page1 = std::make_unique<mx::ui::Column>();
  auto b = std::make_unique<mx::ui::Button>();
  b->text(L"PageB");
  page1->AddChild(std::move(b));
  auto tab = std::make_unique<mx::ui::Tab>();
  mx::ui::Tab* tab_ptr = tab.get();
  tab->AddChild(std::move(page0));
  tab->AddChild(std::move(page1));
  tab->add_header(L"一");
  tab->add_header(L"二");
  tab->acc_name(L"分页");
  tab->set_selected(0);

  auto box = std::make_unique<mx::ui::Checkbox>();
  box->text(L"记住");
  mx::ui::Checkbox* box_ptr = box.get();

  auto slider = std::make_unique<mx::ui::Slider>();
  slider->acc_name(L"音量");
  slider->value(0.2f);
  mx::ui::Slider* slider_ptr = slider.get();

  auto bar = std::make_unique<mx::ui::ProgressBar>();
  bar->acc_name(L"下载");
  bar->value(0.3f);

  auto combo = std::make_unique<mx::ui::Combo>();
  combo->acc_name(L"颜色");
  combo->items({L"红", L"绿"});
  combo->selected(0);
  combo->BindWindow(&window);
  mx::ui::Combo* combo_ptr = combo.get();

  auto root = std::make_unique<mx::ui::Column>();
  root->AddChild(std::move(ok));
  root->AddChild(std::move(hidden));
  root->AddChild(std::move(menu));
  root->AddChild(std::move(tab));
  root->AddChild(std::move(box));
  root->AddChild(std::move(slider));
  root->AddChild(std::move(bar));
  root->AddChild(std::move(combo));
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

  IUIAutomationElement* tab_el = FindByName(uia, root_el, L"分页");
  Expect("find 分页", tab_el != nullptr);
  Expect("分页 is Tab", ControlTypeOf(tab_el) == UIA_TabControlTypeId);
  if (tab_el) {
    IUIAutomationSelectionPattern* sel = nullptr;
    hr = tab_el->GetCurrentPatternAs(UIA_SelectionPatternId,
                                     IID_IUIAutomationSelectionPattern,
                                     reinterpret_cast<void**>(&sel));
    Expect("Tab Selection pattern", SUCCEEDED(hr) && sel);
    if (sel) {
      BOOL multi = TRUE;
      sel->get_CurrentCanSelectMultiple(&multi);
      Expect("Tab single-select", multi == FALSE);
      sel->Release();
    }
    IUIAutomationValuePattern* tab_val = nullptr;
    hr = tab_el->GetCurrentPatternAs(UIA_ValuePatternId,
                                     IID_IUIAutomationValuePattern,
                                     reinterpret_cast<void**>(&tab_val));
    Expect("Tab Value pattern", SUCCEEDED(hr) && tab_val);
    if (tab_val) {
      BSTR one = SysAllocString(L"一");
      Expect("Tab SetValue", SUCCEEDED(tab_val->SetValue(one)));
      SysFreeString(one);
      Pump();
      Expect("Tab AccSetValue selected", tab_ptr->selected() == 0);
      tab_val->Release();
    }
  }

  IUIAutomationElement* slider_el = FindByName(uia, root_el, L"音量");
  Expect("find 音量", slider_el != nullptr);
  Expect("音量 is Slider", ControlTypeOf(slider_el) == UIA_SliderControlTypeId);
  if (slider_el) {
    IUIAutomationRangeValuePattern* range = nullptr;
    hr = slider_el->GetCurrentPatternAs(
        UIA_RangeValuePatternId, IID_IUIAutomationRangeValuePattern,
        reinterpret_cast<void**>(&range));
    Expect("Slider RangeValue", SUCCEEDED(hr) && range);
    if (range) {
      Expect("Slider SetValue", SUCCEEDED(range->SetValue(0.6)));
      Pump();
      double cur = 0.0;
      range->get_CurrentValue(&cur);
      Expect("Slider value", cur > 0.59 && cur < 0.61);
      Expect("Slider node value", slider_ptr->value() > 0.59f &&
                                      slider_ptr->value() < 0.61f);
      range->Release();
    }
  }

  IUIAutomationElement* bar_el = FindByName(uia, root_el, L"下载");
  Expect("find 下载", bar_el != nullptr);
  Expect("下载 is ProgressBar",
         ControlTypeOf(bar_el) == UIA_ProgressBarControlTypeId);
  if (bar_el) {
    IUIAutomationRangeValuePattern* range = nullptr;
    hr = bar_el->GetCurrentPatternAs(UIA_RangeValuePatternId,
                                     IID_IUIAutomationRangeValuePattern,
                                     reinterpret_cast<void**>(&range));
    Expect("Progress RangeValue", SUCCEEDED(hr) && range);
    if (range) {
      BOOL ro = FALSE;
      range->get_CurrentIsReadOnly(&ro);
      Expect("Progress readonly", ro == TRUE);
      Expect("Progress SetValue denied", FAILED(range->SetValue(0.9)));
      range->Release();
    }
  }

  IUIAutomationElement* combo_el = FindByName(uia, root_el, L"颜色");
  Expect("find 颜色", combo_el != nullptr);
  Expect("颜色 is ComboBox", ControlTypeOf(combo_el) == UIA_ComboBoxControlTypeId);
  if (combo_el) {
    IUIAutomationExpandCollapsePattern* exp = nullptr;
    hr = combo_el->GetCurrentPatternAs(
        UIA_ExpandCollapsePatternId, IID_IUIAutomationExpandCollapsePattern,
        reinterpret_cast<void**>(&exp));
    Expect("Combo ExpandCollapse", SUCCEEDED(hr) && exp);
    if (exp) {
      Expect("Combo Expand", SUCCEEDED(exp->Expand()));
      Pump();
      Expect("Combo is open", combo_ptr->is_open());
      Expect("Combo Collapse", SUCCEEDED(exp->Collapse()));
      Pump();
      Expect("Combo is closed", !combo_ptr->is_open());
      exp->Release();
    }
  }

  IUIAutomationElement* box_el = FindByName(uia, root_el, L"记住");
  Expect("find 记住", box_el != nullptr);
  Expect("记住 is CheckBox", ControlTypeOf(box_el) == UIA_CheckBoxControlTypeId);
  if (box_el) {
    PropSink* sink = new PropSink();
    PROPERTYID pid = UIA_ToggleToggleStatePropertyId;
    hr = uia->AddPropertyChangedEventHandlerNativeArray(
        box_el, TreeScope_Element, nullptr, sink, &pid, 1);
    Expect("toggle event handler", SUCCEEDED(hr));
    IUIAutomationTogglePattern* toggle = nullptr;
    hr = box_el->GetCurrentPatternAs(UIA_TogglePatternId,
                                     IID_IUIAutomationTogglePattern,
                                     reinterpret_cast<void**>(&toggle));
    Expect("Toggle pattern", SUCCEEDED(hr) && toggle);
    if (toggle) {
      Expect("Toggle HRESULT", SUCCEEDED(toggle->Toggle()));
      Pump();
      Expect("Toggle checked", box_ptr->checked());
      Expect("Toggle event raised", sink->count() >= 1);
      toggle->Release();
    }
    uia->RemovePropertyChangedEventHandler(box_el, sink);
    sink->Release();
  }

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
  if (tab_el) {
    tab_el->Release();
  }
  if (slider_el) {
    slider_el->Release();
  }
  if (bar_el) {
    bar_el->Release();
  }
  if (combo_el) {
    combo_el->Release();
  }
  if (box_el) {
    box_el->Release();
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
