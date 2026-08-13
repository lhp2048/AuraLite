
#ifndef __view_accessibility_wrapper_h__
#define __view_accessibility_wrapper_h__

#pragma once

#include <oleacc.h>

#include "base/basic_types.h"

namespace view
{
class View;
}

////////////////////////////////////////////////////////////////////////////////
//
// ViewAccessibilityWrapper
//
////////////////////////////////////////////////////////////////////////////////
class ViewAccessibilityWrapper
{
public:
    explicit ViewAccessibilityWrapper(view::View* view);
    ~ViewAccessibilityWrapper() {}

#if defined(AURALITE_HAS_ATL)
    STDMETHODIMP CreateDefaultInstance(REFIID iid);
    HRESULT Uninitialize();
    STDMETHODIMP GetInstance(REFIID iid, void** interface_ptr);
    STDMETHODIMP SetInstance(IAccessible* interface_ptr);
#else
    // Stub without ATL: no MSAA instance is created.
    HRESULT Uninitialize() { return S_OK; }
    HRESULT GetInstance(REFIID iid, void** interface_ptr)
    {
        (void)iid;
        if(interface_ptr)
        {
            *interface_ptr = NULL;
        }
        return E_NOINTERFACE;
    }
#endif

private:
    view::View* view_;
#if defined(AURALITE_HAS_ATL)
    IAccessible* accessibility_iaccessible_;
#endif
    DISALLOW_COPY_AND_ASSIGN(ViewAccessibilityWrapper);
};

#endif //__view_accessibility_wrapper_h__
