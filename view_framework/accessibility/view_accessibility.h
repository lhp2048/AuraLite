
#ifndef __view_accessibility_h__
#define __view_accessibility_h__

#pragma once

// ATL is optional. Without Microsoft.VisualStudio.Component.VC.ATLMFC,
// AuraLite builds a stub that keeps MSAA role/state/event mapping only.
#if defined(AURALITE_HAS_ATL)
#include <atlbase.h>
#include <atlcom.h>
#endif

#include <oleacc.h>

#include "accessibility_types.h"
#include "base/basic_types.h"

namespace view
{
class View;
}

class ViewAccessibilityWrapper;

////////////////////////////////////////////////////////////////////////////////
//
// ViewAccessibility
//
////////////////////////////////////////////////////////////////////////////////
#if defined(AURALITE_HAS_ATL)

class ATL_NO_VTABLE ViewAccessibility
    : public CComObjectRootEx<CComMultiThreadModel>,
      public IDispatchImpl<IAccessible, &IID_IAccessible, &LIBID_Accessibility>
{
public:
    BEGIN_COM_MAP(ViewAccessibility)
        COM_INTERFACE_ENTRY2(IDispatch, IAccessible)
        COM_INTERFACE_ENTRY(IAccessible)
    END_COM_MAP()

    ViewAccessibility() {}
    ~ViewAccessibility() {}

    HRESULT Initialize(view::View* view);

    // IAccessible (see view_accessibility.cpp)
    STDMETHODIMP accHitTest(LONG x_left, LONG y_top, VARIANT* child);
    STDMETHODIMP accLocation(LONG* x_left, LONG* y_top, LONG* width,
                             LONG* height, VARIANT var_id);
    STDMETHODIMP accNavigate(LONG nav_dir, VARIANT start, VARIANT* end);
    STDMETHODIMP get_accChild(VARIANT var_child, IDispatch** disp_child);
    STDMETHODIMP get_accChildCount(LONG* child_count);
    STDMETHODIMP get_accDefaultAction(VARIANT var_id, BSTR* default_action);
    STDMETHODIMP get_accDescription(VARIANT var_id, BSTR* desc);
    STDMETHODIMP get_accFocus(VARIANT* focus_child);
    STDMETHODIMP get_accKeyboardShortcut(VARIANT var_id, BSTR* access_key);
    STDMETHODIMP get_accName(VARIANT var_id, BSTR* name);
    STDMETHODIMP get_accParent(IDispatch** disp_parent);
    STDMETHODIMP get_accRole(VARIANT var_id, VARIANT* role);
    STDMETHODIMP get_accState(VARIANT var_id, VARIANT* state);
    STDMETHODIMP get_accValue(VARIANT var_id, BSTR* value);
    STDMETHODIMP accDoDefaultAction(VARIANT var_id);
    STDMETHODIMP get_accSelection(VARIANT* selected);
    STDMETHODIMP accSelect(LONG flagsSelect, VARIANT var_id);
    STDMETHODIMP get_accHelp(VARIANT var_id, BSTR* help);
    STDMETHODIMP get_accHelpTopic(BSTR* help_file, VARIANT var_id, LONG* topic);
    STDMETHODIMP put_accName(VARIANT var_id, BSTR put_name);
    STDMETHODIMP put_accValue(VARIANT var_id, BSTR put_val);

    static int32 MSAAEvent(AccessibilityTypes::Event event);
    static int32 MSAARole(AccessibilityTypes::Role role);
    static int32 MSAAState(AccessibilityTypes::State state);

private:
    ViewAccessibilityWrapper* GetViewAccessibilityWrapper(view::View* v) const;
    bool IsNavDirNext(int nav_dir) const;
    bool IsValidNav(int nav_dir, int start_id, int lower_bound,
                    int upper_bound) const;
    bool IsValidId(const VARIANT& child) const;
    void SetState(VARIANT* msaa_state, view::View* view);
    HRESULT GetNativeIAccessibleInterface(view::View* view, IAccessible** acc);
    HRESULT GetNativeIAccessibleInterface(IAccessible* view, VARIANT* acc);

    view::View* view_;
    DISALLOW_COPY_AND_ASSIGN(ViewAccessibility);
};

#else  // !AURALITE_HAS_ATL

class ViewAccessibility
{
public:
    static int32 MSAAEvent(AccessibilityTypes::Event event);
    static int32 MSAARole(AccessibilityTypes::Role role);
    static int32 MSAAState(AccessibilityTypes::State state);

private:
    ViewAccessibility();
    DISALLOW_COPY_AND_ASSIGN(ViewAccessibility);
};

#endif  // AURALITE_HAS_ATL

#endif //__view_accessibility_h__
