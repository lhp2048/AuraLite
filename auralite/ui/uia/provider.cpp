#include "auralite/ui/uia/provider.h"

#include "auralite/ui/acc.h"
#include "auralite/ui/node.h"
#include "auralite/ui/window.h"

#include <oleacc.h>
#include <oleauto.h>
#include <UIAutomation.h>
#include <UIAutomationCoreApi.h>

#include <cmath>
#include <string>
#include <vector>

#pragma comment(lib, "UIAutomationCore.lib")

namespace auralite::ui {
namespace {

HRESULT SetBstr(VARIANT* out, const std::wstring& s) {
  if (!out) {
    return E_POINTER;
  }
  out->vt = VT_BSTR;
  out->bstrVal = SysAllocStringLen(s.c_str(), static_cast<UINT>(s.size()));
  return (out->bstrVal || s.empty()) ? S_OK : E_OUTOFMEMORY;
}

HRESULT SetBool(VARIANT* out, bool v) {
  if (!out) {
    return E_POINTER;
  }
  out->vt = VT_BOOL;
  out->boolVal = v ? VARIANT_TRUE : VARIANT_FALSE;
  return S_OK;
}

HRESULT SetI4(VARIANT* out, int v) {
  if (!out) {
    return E_POINTER;
  }
  out->vt = VT_I4;
  out->lVal = v;
  return S_OK;
}

std::wstring Utf8Wide(const std::string& utf8) {
  if (utf8.empty()) {
    return {};
  }
  const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                    static_cast<int>(utf8.size()), nullptr, 0);
  if (n <= 0) {
    return {};
  }
  std::wstring out(static_cast<size_t>(n), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()),
                      out.data(), n);
  return out;
}

HRESULT SetR8(VARIANT* out, double v) {
  if (!out) {
    return E_POINTER;
  }
  out->vt = VT_R8;
  out->dblVal = v;
  return S_OK;
}

CONTROLTYPEID ControlTypeOf(AccRole role, bool is_root) {
  if (is_root) {
    return UIA_WindowControlTypeId;
  }
  switch (role) {
    case AccRole::Button:
      return UIA_ButtonControlTypeId;
    case AccRole::Text:
      return UIA_TextControlTypeId;
    case AccRole::Edit:
      return UIA_EditControlTypeId;
    case AccRole::CheckBox:
      return UIA_CheckBoxControlTypeId;
    case AccRole::RadioButton:
      return UIA_RadioButtonControlTypeId;
    case AccRole::ComboBox:
      return UIA_ComboBoxControlTypeId;
    case AccRole::MenuItem:
      return UIA_MenuItemControlTypeId;
    case AccRole::Slider:
      return UIA_SliderControlTypeId;
    case AccRole::ProgressBar:
      return UIA_ProgressBarControlTypeId;
    case AccRole::Tab:
      return UIA_TabControlTypeId;
    case AccRole::List:
      return UIA_ListControlTypeId;
    case AccRole::Tree:
      return UIA_TreeControlTypeId;
    case AccRole::Spinner:
      return UIA_SpinnerControlTypeId;
    case AccRole::MenuBar:
      return UIA_MenuBarControlTypeId;
    case AccRole::StatusBar:
      return UIA_StatusBarControlTypeId;
    case AccRole::Group:
      return UIA_GroupControlTypeId;
    case AccRole::Ignore:
    default:
      return UIA_CustomControlTypeId;
  }
}

}  // namespace

bool IsUiaGetObject(LPARAM lparam) {
  const LONG obj = static_cast<LONG>(lparam);
  return obj == static_cast<LONG>(UiaRootObjectId) || obj == OBJID_CLIENT;
}

class UiaProvider : public IRawElementProviderSimple,
                    public IRawElementProviderFragment,
                    public IRawElementProviderFragmentRoot,
                    public IInvokeProvider,
                    public IValueProvider,
                    public IToggleProvider,
                    public IRangeValueProvider,
                    public IExpandCollapseProvider,
                    public ISelectionProvider {
 public:
  UiaProvider(Window* window, int acc_id)
      : window_(window),
        alive_(window ? window->alive_flag() : nullptr),
        acc_id_(acc_id) {}

  UiaProvider(const UiaProvider&) = delete;
  UiaProvider& operator=(const UiaProvider&) = delete;

  friend class Window;

  // IUnknown
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
    *ppv = nullptr;
    Node* node = nullptr;
    const bool patterns = acc_id_ != 0 && Host() && (node = NodeOf());
    if (riid == IID_IUnknown || riid == IID_IRawElementProviderSimple) {
      *ppv = static_cast<IRawElementProviderSimple*>(this);
    } else if (riid == IID_IRawElementProviderFragment) {
      *ppv = static_cast<IRawElementProviderFragment*>(this);
    } else if (riid == IID_IRawElementProviderFragmentRoot && acc_id_ == 0) {
      *ppv = static_cast<IRawElementProviderFragmentRoot*>(this);
    } else if (riid == IID_IInvokeProvider && patterns && HasInvoke(*node)) {
      *ppv = static_cast<IInvokeProvider*>(this);
    } else if (riid == IID_IValueProvider && patterns && HasValue(*node)) {
      *ppv = static_cast<IValueProvider*>(this);
    } else if (riid == IID_IToggleProvider && patterns && HasToggle(*node)) {
      *ppv = static_cast<IToggleProvider*>(this);
    } else if (riid == IID_IRangeValueProvider && patterns && HasRange(*node)) {
      *ppv = static_cast<IRangeValueProvider*>(this);
    } else if (riid == IID_IExpandCollapseProvider && patterns &&
               HasExpand(*node)) {
      *ppv = static_cast<IExpandCollapseProvider*>(this);
    } else if (riid == IID_ISelectionProvider && patterns &&
               HasSelection(*node)) {
      *ppv = static_cast<ISelectionProvider*>(this);
    } else {
      return E_NOINTERFACE;
    }
    AddRef();
    return S_OK;
  }

  // IRawElementProviderSimple
  HRESULT STDMETHODCALLTYPE get_ProviderOptions(
      ProviderOptions* pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    *pRetVal = static_cast<ProviderOptions>(
        ProviderOptions_ServerSideProvider | ProviderOptions_UseComThreading);
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID patternId,
                                               IUnknown** pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    *pRetVal = nullptr;
    Node* node = NodeOf();
    if (!Host() || (acc_id_ != 0 && !node)) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    if (acc_id_ == 0) {
      return S_OK;
    }
    if (patternId == UIA_InvokePatternId && HasInvoke(*node)) {
      *pRetVal = static_cast<IInvokeProvider*>(this);
      AddRef();
    } else if (patternId == UIA_ValuePatternId && HasValue(*node)) {
      *pRetVal = static_cast<IValueProvider*>(this);
      AddRef();
    } else if (patternId == UIA_TogglePatternId && HasToggle(*node)) {
      *pRetVal = static_cast<IToggleProvider*>(this);
      AddRef();
    } else if (patternId == UIA_RangeValuePatternId && HasRange(*node)) {
      *pRetVal = static_cast<IRangeValueProvider*>(this);
      AddRef();
    } else if (patternId == UIA_ExpandCollapsePatternId && HasExpand(*node)) {
      *pRetVal = static_cast<IExpandCollapseProvider*>(this);
      AddRef();
    } else if (patternId == UIA_SelectionPatternId && HasSelection(*node)) {
      *pRetVal = static_cast<ISelectionProvider*>(this);
      AddRef();
    }
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID propertyId,
                                             VARIANT* pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    VariantInit(pRetVal);
    Window* w = Host();
    if (!w || !w->hwnd_) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    Node* node = NodeOf();
    if (acc_id_ != 0 && !node) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }

    if (propertyId == UIA_NamePropertyId) {
      return SetBstr(pRetVal, acc_id_ == 0 ? WindowTitle(w) : node->AccName());
    }
    if (propertyId == UIA_ControlTypePropertyId) {
      const AccRole role = node ? node->acc_role() : AccRole::Ignore;
      return SetI4(pRetVal, static_cast<int>(ControlTypeOf(role, acc_id_ == 0)));
    }
    if (propertyId == UIA_IsEnabledPropertyId) {
      const bool on = acc_id_ == 0 || (node && !node->acc_state().disabled);
      return SetBool(pRetVal, on);
    }
    if (propertyId == UIA_HasKeyboardFocusPropertyId) {
      const bool has = ::GetFocus() == w->hwnd_ &&
                       ((acc_id_ == 0 && (!w->focused_ || !w->focused_->AccIncluded())) ||
                        (node && node->focused()));
      return SetBool(pRetVal, has);
    }
    if (propertyId == UIA_IsKeyboardFocusablePropertyId) {
      if (acc_id_ == 0) {
        return SetBool(pRetVal, true);
      }
      return SetBool(pRetVal, node && node->focusable() && !node->acc_state().disabled);
    }
    if (propertyId == UIA_IsControlElementPropertyId ||
        propertyId == UIA_IsContentElementPropertyId) {
      return SetBool(pRetVal, true);
    }
    if (propertyId == UIA_IsPasswordPropertyId) {
      return SetBool(pRetVal, node && node->acc_state().password);
    }
    if (propertyId == UIA_AutomationIdPropertyId && node && !node->name().empty()) {
      return SetBstr(pRetVal, Utf8Wide(node->name()));
    }
    if (propertyId == UIA_HelpTextPropertyId && node && !node->tooltip().empty() &&
        node->tooltip() != node->AccName()) {
      return SetBstr(pRetVal, node->tooltip());
    }
    if (propertyId == UIA_ValueValuePropertyId && node && HasValue(*node)) {
      return SetBstr(pRetVal, node->AccValue());
    }
    if (propertyId == UIA_RangeValueValuePropertyId && node && HasRange(*node)) {
      return SetR8(pRetVal, node->AccRangeValue());
    }
    if (propertyId == UIA_RangeValueIsReadOnlyPropertyId && node &&
        HasRange(*node)) {
      return SetBool(pRetVal, node->AccRangeReadOnly());
    }
    if (propertyId == UIA_ExpandCollapseExpandCollapseStatePropertyId && node &&
        HasExpand(*node)) {
      return SetI4(pRetVal, node->AccIsExpanded()
                                ? ExpandCollapseState_Expanded
                                : ExpandCollapseState_Collapsed);
    }
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(
      IRawElementProviderSimple** pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    *pRetVal = nullptr;
    Window* w = Host();
    if (!w || !w->hwnd_) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    if (acc_id_ != 0) {
      return S_OK;
    }
    return UiaHostProviderFromHwnd(w->hwnd_, pRetVal);
  }

  // IRawElementProviderFragment
  HRESULT STDMETHODCALLTYPE Navigate(NavigateDirection direction,
                                     IRawElementProviderFragment** pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    *pRetVal = nullptr;
    Window* w = Host();
    if (!w) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    std::vector<Node*> items;
    Collect(w, &items);

    if (acc_id_ == 0) {
      if (items.empty()) {
        return S_OK;
      }
      if (direction == NavigateDirection_FirstChild) {
        *pRetVal = ForNode(w, items.front());
      } else if (direction == NavigateDirection_LastChild) {
        *pRetVal = ForNode(w, items.back());
      }
      return S_OK;
    }

    if (direction == NavigateDirection_Parent) {
      *pRetVal = RootProvider(w);
      return S_OK;
    }
    if (direction == NavigateDirection_FirstChild ||
        direction == NavigateDirection_LastChild) {
      return S_OK;
    }

    int index = -1;
    for (size_t i = 0; i < items.size(); ++i) {
      if (items[i]->acc_id() == acc_id_) {
        index = static_cast<int>(i);
        break;
      }
    }
    if (index < 0) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    if (direction == NavigateDirection_NextSibling &&
        index + 1 < static_cast<int>(items.size())) {
      *pRetVal = ForNode(w, items[static_cast<size_t>(index + 1)]);
    } else if (direction == NavigateDirection_PreviousSibling && index > 0) {
      *pRetVal = ForNode(w, items[static_cast<size_t>(index - 1)]);
    }
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE GetRuntimeId(SAFEARRAY** pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    *pRetVal = nullptr;
    if (acc_id_ == 0) {
      return S_OK;
    }
    if (!Host()) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    SAFEARRAY* sa = SafeArrayCreateVector(VT_I4, 0, 2);
    if (!sa) {
      return E_OUTOFMEMORY;
    }
    LONG i0 = 0;
    int v0 = UiaAppendRuntimeId;
    SafeArrayPutElement(sa, &i0, &v0);
    LONG i1 = 1;
    int v1 = acc_id_;
    SafeArrayPutElement(sa, &i1, &v1);
    *pRetVal = sa;
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE get_BoundingRectangle(UiaRect* pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    *pRetVal = UiaRect{0, 0, 0, 0};
    Window* w = Host();
    if (!w || !w->hwnd_) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    if (acc_id_ == 0) {
      RECT r{};
      GetWindowRect(w->hwnd_, &r);
      pRetVal->left = r.left;
      pRetVal->top = r.top;
      pRetVal->width = r.right - r.left;
      pRetVal->height = r.bottom - r.top;
      return S_OK;
    }
    Node* node = NodeOf();
    if (!node) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    const RectF b = node->bounds();
    const float dpi = w->dpi_;
    POINT tl{static_cast<LONG>(std::floor(auralite::PxFromDip(b.x, dpi))),
             static_cast<LONG>(std::floor(auralite::PxFromDip(b.y, dpi)))};
    POINT br{static_cast<LONG>(std::ceil(auralite::PxFromDip(b.x + b.w, dpi))),
             static_cast<LONG>(std::ceil(auralite::PxFromDip(b.y + b.h, dpi)))};
    ClientToScreen(w->hwnd_, &tl);
    ClientToScreen(w->hwnd_, &br);
    pRetVal->left = tl.x;
    pRetVal->top = tl.y;
    pRetVal->width = br.x - tl.x;
    pRetVal->height = br.y - tl.y;
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    *pRetVal = nullptr;
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE SetFocus() override {
    Window* w = Host();
    if (!w || !w->hwnd_) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    if (acc_id_ == 0) {
      ::SetFocus(w->hwnd_);
      return S_OK;
    }
    Node* node = NodeOf();
    if (!node) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    if (node->focusable()) {
      w->SetFocusNode(node);
    }
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE get_FragmentRoot(
      IRawElementProviderFragmentRoot** pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    *pRetVal = nullptr;
    Window* w = Host();
    if (!w) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    UiaProvider* root = RootProvider(w);
    if (!root) {
      return E_FAIL;
    }
    *pRetVal = static_cast<IRawElementProviderFragmentRoot*>(root);
    return S_OK;
  }

  // IRawElementProviderFragmentRoot
  HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(
      double x, double y, IRawElementProviderFragment** pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    *pRetVal = nullptr;
    Window* w = Host();
    if (!w || !w->hwnd_ || acc_id_ != 0) {
      return acc_id_ == 0 ? UIA_E_ELEMENTNOTAVAILABLE : S_OK;
    }
    POINT pt{static_cast<LONG>(x), static_cast<LONG>(y)};
    ScreenToClient(w->hwnd_, &pt);
    const float dx = auralite::DipFromPx(static_cast<float>(pt.x), w->dpi_);
    const float dy = auralite::DipFromPx(static_cast<float>(pt.y), w->dpi_);
    Node* hit = nullptr;
    if (w->popup_) {
      hit = w->popup_->HitTest(dx, dy);
    }
    if (!hit && w->root_) {
      hit = w->root_->HitTest(dx, dy);
    }
    for (Node* n = hit; n; n = n->parent()) {
      if (n->AccIncluded()) {
        *pRetVal = ForNode(w, n);
        return S_OK;
      }
    }
    AddRef();
    *pRetVal = this;
    return S_OK;
  }

  HRESULT STDMETHODCALLTYPE GetFocus(IRawElementProviderFragment** pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    *pRetVal = nullptr;
    Window* w = Host();
    if (!w || acc_id_ != 0) {
      return acc_id_ == 0 ? UIA_E_ELEMENTNOTAVAILABLE : S_OK;
    }
    if (w->focused_ && w->focused_->AccIncluded()) {
      *pRetVal = ForNode(w, w->focused_);
      return S_OK;
    }
    AddRef();
    *pRetVal = this;
    return S_OK;
  }

  // IInvokeProvider
  HRESULT STDMETHODCALLTYPE Invoke() override {
    Node* node = NodeOf();
    Window* w = Host();
    if (!w || !node) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    if (!node->AccInvoke()) {
      return UIA_E_INVALIDOPERATION;
    }
    w->Invalidate();
    UiaRaiseAutomationEvent(static_cast<IRawElementProviderSimple*>(this),
                            UIA_Invoke_InvokedEventId);
    return S_OK;
  }

  // IValueProvider
  HRESULT STDMETHODCALLTYPE SetValue(LPCWSTR val) override {
    Node* node = NodeOf();
    Window* w = Host();
    if (!w || !node) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    if (!node->AccSetValue(val ? val : L"")) {
      return UIA_E_INVALIDOPERATION;
    }
    w->Invalidate();
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE get_Value(BSTR* pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    *pRetVal = nullptr;
    Node* node = NodeOf();
    if (!node) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    const std::wstring v = node->AccValue();
    *pRetVal = SysAllocStringLen(v.c_str(), static_cast<UINT>(v.size()));
    return (*pRetVal || v.empty()) ? S_OK : E_OUTOFMEMORY;
  }
  HRESULT STDMETHODCALLTYPE get_IsReadOnly(BOOL* pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    Node* node = NodeOf();
    if (!node) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    *pRetVal = HasRange(*node) && node->AccRangeReadOnly() ? TRUE : FALSE;
    return S_OK;
  }

  // IToggleProvider
  HRESULT STDMETHODCALLTYPE Toggle() override {
    Node* node = NodeOf();
    Window* w = Host();
    if (!w || !node) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    if (!node->AccToggle()) {
      return UIA_E_INVALIDOPERATION;
    }
    w->Invalidate();
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE get_ToggleState(ToggleState* pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    Node* node = NodeOf();
    if (!node) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    *pRetVal = node->acc_state().checked ? ToggleState_On : ToggleState_Off;
    return S_OK;
  }

  // IRangeValueProvider
  HRESULT STDMETHODCALLTYPE SetValue(double val) override {
    Node* node = NodeOf();
    Window* w = Host();
    if (!w || !node) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    if (node->AccRangeReadOnly() || !node->AccSetRangeValue(val)) {
      return UIA_E_INVALIDOPERATION;
    }
    w->Invalidate();
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE get_Value(double* pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    Node* node = NodeOf();
    if (!node) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    *pRetVal = node->AccRangeValue();
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE get_Maximum(double* pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    Node* node = NodeOf();
    if (!node) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    *pRetVal = node->AccRangeMaximum();
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE get_Minimum(double* pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    Node* node = NodeOf();
    if (!node) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    *pRetVal = node->AccRangeMinimum();
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE get_LargeChange(double* pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    Node* node = NodeOf();
    if (!node) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    *pRetVal = node->AccRangeLargeChange();
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE get_SmallChange(double* pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    Node* node = NodeOf();
    if (!node) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    *pRetVal = node->AccRangeSmallChange();
    return S_OK;
  }

  // IExpandCollapseProvider
  HRESULT STDMETHODCALLTYPE Expand() override {
    Node* node = NodeOf();
    Window* w = Host();
    if (!w || !node) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    if (!node->AccExpand()) {
      return UIA_E_INVALIDOPERATION;
    }
    w->Invalidate();
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE Collapse() override {
    Node* node = NodeOf();
    Window* w = Host();
    if (!w || !node) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    if (!node->AccCollapse()) {
      return UIA_E_INVALIDOPERATION;
    }
    w->Invalidate();
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE get_ExpandCollapseState(
      ExpandCollapseState* pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    Node* node = NodeOf();
    if (!node) {
      return UIA_E_ELEMENTNOTAVAILABLE;
    }
    *pRetVal = node->AccIsExpanded() ? ExpandCollapseState_Expanded
                                     : ExpandCollapseState_Collapsed;
    return S_OK;
  }

  // ISelectionProvider
  HRESULT STDMETHODCALLTYPE GetSelection(SAFEARRAY** pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    *pRetVal = SafeArrayCreateVector(VT_UNKNOWN, 0, 0);
    return *pRetVal ? S_OK : E_OUTOFMEMORY;
  }
  HRESULT STDMETHODCALLTYPE get_CanSelectMultiple(BOOL* pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    *pRetVal = FALSE;
    return S_OK;
  }
  HRESULT STDMETHODCALLTYPE get_IsSelectionRequired(BOOL* pRetVal) override {
    if (!pRetVal) {
      return E_POINTER;
    }
    *pRetVal = TRUE;
    return S_OK;
  }

 private:
  static bool HasInvoke(const Node& n) {
    return n.acc_role() == AccRole::Button ||
           n.acc_role() == AccRole::MenuItem;
  }
  static bool HasValue(const Node& n) {
    return n.acc_role() == AccRole::Edit || n.acc_role() == AccRole::ComboBox ||
           n.acc_role() == AccRole::Tab;
  }
  static bool HasToggle(const Node& n) {
    return n.acc_role() == AccRole::CheckBox ||
           n.acc_role() == AccRole::RadioButton;
  }
  static bool HasRange(const Node& n) {
    return n.acc_role() == AccRole::Slider ||
           n.acc_role() == AccRole::ProgressBar ||
           n.acc_role() == AccRole::Spinner;
  }
  static bool HasExpand(const Node& n) {
    return n.acc_role() == AccRole::ComboBox;
  }
  static bool HasSelection(const Node& n) {
    return n.acc_role() == AccRole::Tab;
  }

  Window* Host() const {
    if (!alive_ || !alive_->load() || !window_) {
      return nullptr;
    }
    return window_;
  }

  static void Collect(Window* w, std::vector<Node*>* out) {
    CollectAccNodes(w->root_.get(), out);
    CollectAccNodes(w->popup_.get(), out);
  }

  static Node* Find(Window* w, int id) {
    if (Node* h = FindAccNode(w->root_.get(), id)) {
      return h;
    }
    return FindAccNode(w->popup_.get(), id);
  }

  Node* NodeOf() const {
    Window* w = Host();
    if (!w || acc_id_ == 0) {
      return nullptr;
    }
    Node* n = Find(w, acc_id_);
    if (!n || !n->AccIncluded()) {
      return nullptr;
    }
    return n;
  }

  static std::wstring WindowTitle(Window* w) {
    wchar_t buf[512] = {};
    GetWindowTextW(w->hwnd_, buf, 512);
    return buf;
  }

  static UiaProvider* ForNode(Window* w, Node* n) {
    n->EnsureAccId();
    return new UiaProvider(w, n->acc_id());
  }

  static IRawElementProviderSimple* SimpleOf(Window* w, Node* node) {
    if (!w || !w->uia_root_ || !w->hwnd_ || !node || !node->AccIncluded()) {
      return nullptr;
    }
    return static_cast<IRawElementProviderSimple*>(ForNode(w, node));
  }

  static UiaProvider* RootProvider(Window* w) {
    if (!w->uia_root_) {
      w->uia_root_ = new UiaProvider(w, 0);
    }
    w->uia_root_->AddRef();
    return w->uia_root_;
  }

  Window* window_ = nullptr;
  std::shared_ptr<std::atomic_bool> alive_;
  int acc_id_ = 0;
  LONG ref_ = 1;
};

LRESULT Window::HandleGetObject(WPARAM wparam, LPARAM lparam) {
  if (!hwnd_) {
    return 0;
  }
  if (!uia_root_) {
    uia_root_ = new UiaProvider(this, 0);
  }
  return UiaReturnRawElementProvider(
      hwnd_, wparam, lparam,
      static_cast<IRawElementProviderSimple*>(uia_root_));
}

void Window::DisconnectUia() {
  if (!uia_root_) {
    return;
  }
  UiaDisconnectProvider(static_cast<IRawElementProviderSimple*>(uia_root_));
  uia_root_->Release();
  uia_root_ = nullptr;
}

void Window::RaiseAccFocusChanged() {
  if (!uia_root_ || !hwnd_) {
    return;
  }
  IRawElementProviderSimple* p = nullptr;
  if (focused_ && focused_->AccIncluded()) {
    p = static_cast<IRawElementProviderSimple*>(
        UiaProvider::ForNode(this, focused_));
  } else {
    uia_root_->AddRef();
    p = static_cast<IRawElementProviderSimple*>(uia_root_);
  }
  UiaRaiseAutomationEvent(p, UIA_AutomationFocusChangedEventId);
  p->Release();
}

void Window::RaiseAccToggleChanged(Node* node) {
  IRawElementProviderSimple* p = UiaProvider::SimpleOf(this, node);
  if (!p) {
    return;
  }
  VARIANT oldv;
  VARIANT newv;
  VariantInit(&oldv);
  VariantInit(&newv);
  newv.vt = VT_I4;
  newv.lVal = node->acc_state().checked ? ToggleState_On : ToggleState_Off;
  UiaRaiseAutomationPropertyChangedEvent(p, UIA_ToggleToggleStatePropertyId,
                                         oldv, newv);
  p->Release();
}

void Window::RaiseAccValueChanged(Node* node) {
  IRawElementProviderSimple* p = UiaProvider::SimpleOf(this, node);
  if (!p) {
    return;
  }
  VARIANT oldv;
  VARIANT newv;
  VariantInit(&oldv);
  SetBstr(&newv, node->AccValue());
  UiaRaiseAutomationPropertyChangedEvent(p, UIA_ValueValuePropertyId, oldv,
                                         newv);
  VariantClear(&newv);
  p->Release();
}

void Window::RaiseAccRangeChanged(Node* node) {
  IRawElementProviderSimple* p = UiaProvider::SimpleOf(this, node);
  if (!p) {
    return;
  }
  VARIANT oldv;
  VARIANT newv;
  VariantInit(&oldv);
  SetR8(&newv, node->AccRangeValue());
  UiaRaiseAutomationPropertyChangedEvent(p, UIA_RangeValueValuePropertyId, oldv,
                                         newv);
  p->Release();
}

void Window::RaiseAccExpandCollapseChanged(Node* node) {
  IRawElementProviderSimple* p = UiaProvider::SimpleOf(this, node);
  if (!p) {
    return;
  }
  VARIANT oldv;
  VARIANT newv;
  VariantInit(&oldv);
  VariantInit(&newv);
  newv.vt = VT_I4;
  newv.lVal = node->AccIsExpanded() ? ExpandCollapseState_Expanded
                                    : ExpandCollapseState_Collapsed;
  UiaRaiseAutomationPropertyChangedEvent(
      p, UIA_ExpandCollapseExpandCollapseStatePropertyId, oldv, newv);
  p->Release();
}

void Window::RaiseAccStructureChanged() {
  if (!uia_root_ || !hwnd_) {
    return;
  }
  uia_root_->AddRef();
  IRawElementProviderSimple* p =
      static_cast<IRawElementProviderSimple*>(uia_root_);
  UiaRaiseStructureChangedEvent(p, StructureChangeType_ChildrenInvalidated,
                                nullptr, 0);
  p->Release();
}

}  // namespace auralite::ui
