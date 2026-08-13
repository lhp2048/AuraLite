
#include "view_accessibility_wrapper.h"

ViewAccessibilityWrapper::ViewAccessibilityWrapper(view::View* view)
    : view_(view)
#if defined(AURALITE_HAS_ATL)
    , accessibility_iaccessible_(NULL)
#endif
{
    (void)view_;
}

#if defined(AURALITE_HAS_ATL)
#include "view_accessibility.h"

STDMETHODIMP ViewAccessibilityWrapper::CreateDefaultInstance(REFIID iid)
{
    if(!accessibility_iaccessible_)
    {
        CComObject<ViewAccessibility>* instance = NULL;
        HRESULT hr = CComObject<ViewAccessibility>::CreateInstance(&instance);
        if(FAILED(hr) || !instance)
        {
            return E_FAIL;
        }
        instance->Initialize(view_);
        accessibility_iaccessible_ = instance;
        accessibility_iaccessible_->AddRef();
    }
    return accessibility_iaccessible_->QueryInterface(iid,
        reinterpret_cast<void**>(&accessibility_iaccessible_));
}

HRESULT ViewAccessibilityWrapper::Uninitialize()
{
    if(accessibility_iaccessible_)
    {
        accessibility_iaccessible_->Release();
        accessibility_iaccessible_ = NULL;
    }
    return S_OK;
}

STDMETHODIMP ViewAccessibilityWrapper::GetInstance(REFIID iid,
                                                   void** interface_ptr)
{
    if(!accessibility_iaccessible_)
    {
        CreateDefaultInstance(iid);
    }
    if(!accessibility_iaccessible_)
    {
        *interface_ptr = NULL;
        return E_NOINTERFACE;
    }
    return accessibility_iaccessible_->QueryInterface(iid, interface_ptr);
}

STDMETHODIMP ViewAccessibilityWrapper::SetInstance(IAccessible* interface_ptr)
{
    if(accessibility_iaccessible_)
    {
        accessibility_iaccessible_->Release();
    }
    accessibility_iaccessible_ = interface_ptr;
    if(accessibility_iaccessible_)
    {
        accessibility_iaccessible_->AddRef();
    }
    return S_OK;
}
#endif  // AURALITE_HAS_ATL
