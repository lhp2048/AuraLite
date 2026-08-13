
#include "view_accessibility.h"

#include <oleacc.h>

int32 ViewAccessibility::MSAAEvent(AccessibilityTypes::Event event)
{
    switch(event)
    {
    case AccessibilityTypes::EVENT_ALERT:
        return EVENT_SYSTEM_ALERT;
    case AccessibilityTypes::EVENT_FOCUS:
        return EVENT_OBJECT_FOCUS;
    case AccessibilityTypes::EVENT_MENUSTART:
        return EVENT_SYSTEM_MENUSTART;
    case AccessibilityTypes::EVENT_MENUEND:
        return EVENT_SYSTEM_MENUEND;
    case AccessibilityTypes::EVENT_MENUPOPUPSTART:
        return EVENT_SYSTEM_MENUPOPUPSTART;
    case AccessibilityTypes::EVENT_MENUPOPUPEND:
        return EVENT_SYSTEM_MENUPOPUPEND;
    default:
        return EVENT_OBJECT_FOCUS;
    }
}

int32 ViewAccessibility::MSAARole(AccessibilityTypes::Role role)
{
    switch(role)
    {
    case AccessibilityTypes::ROLE_ALERT: return ROLE_SYSTEM_ALERT;
    case AccessibilityTypes::ROLE_APPLICATION: return ROLE_SYSTEM_APPLICATION;
    case AccessibilityTypes::ROLE_BUTTONDROPDOWN: return ROLE_SYSTEM_BUTTONDROPDOWN;
    case AccessibilityTypes::ROLE_BUTTONMENU: return ROLE_SYSTEM_BUTTONMENU;
    case AccessibilityTypes::ROLE_CHECKBUTTON: return ROLE_SYSTEM_CHECKBUTTON;
    case AccessibilityTypes::ROLE_CLIENT: return ROLE_SYSTEM_CLIENT;
    case AccessibilityTypes::ROLE_COMBOBOX: return ROLE_SYSTEM_COMBOBOX;
    case AccessibilityTypes::ROLE_DIALOG: return ROLE_SYSTEM_DIALOG;
    case AccessibilityTypes::ROLE_GRAPHIC: return ROLE_SYSTEM_GRAPHIC;
    case AccessibilityTypes::ROLE_GROUPING: return ROLE_SYSTEM_GROUPING;
    case AccessibilityTypes::ROLE_LINK: return ROLE_SYSTEM_LINK;
    case AccessibilityTypes::ROLE_MENUBAR: return ROLE_SYSTEM_MENUBAR;
    case AccessibilityTypes::ROLE_MENUITEM: return ROLE_SYSTEM_MENUITEM;
    case AccessibilityTypes::ROLE_MENUPOPUP: return ROLE_SYSTEM_MENUPOPUP;
    case AccessibilityTypes::ROLE_OUTLINE: return ROLE_SYSTEM_OUTLINE;
    case AccessibilityTypes::ROLE_OUTLINEITEM: return ROLE_SYSTEM_OUTLINEITEM;
    case AccessibilityTypes::ROLE_PAGETAB: return ROLE_SYSTEM_PAGETAB;
    case AccessibilityTypes::ROLE_PAGETABLIST: return ROLE_SYSTEM_PAGETABLIST;
    case AccessibilityTypes::ROLE_PANE: return ROLE_SYSTEM_PANE;
    case AccessibilityTypes::ROLE_PROGRESSBAR: return ROLE_SYSTEM_PROGRESSBAR;
    case AccessibilityTypes::ROLE_PUSHBUTTON: return ROLE_SYSTEM_PUSHBUTTON;
    case AccessibilityTypes::ROLE_RADIOBUTTON: return ROLE_SYSTEM_RADIOBUTTON;
    case AccessibilityTypes::ROLE_SCROLLBAR: return ROLE_SYSTEM_SCROLLBAR;
    case AccessibilityTypes::ROLE_SEPARATOR: return ROLE_SYSTEM_SEPARATOR;
    case AccessibilityTypes::ROLE_STATICTEXT: return ROLE_SYSTEM_STATICTEXT;
    case AccessibilityTypes::ROLE_TEXT: return ROLE_SYSTEM_TEXT;
    case AccessibilityTypes::ROLE_TITLEBAR: return ROLE_SYSTEM_TITLEBAR;
    case AccessibilityTypes::ROLE_TOOLBAR: return ROLE_SYSTEM_TOOLBAR;
    case AccessibilityTypes::ROLE_WINDOW: return ROLE_SYSTEM_WINDOW;
    default: return ROLE_SYSTEM_CLIENT;
    }
}

int32 ViewAccessibility::MSAAState(AccessibilityTypes::State state)
{
    int32 msaa = 0;
    if(state & AccessibilityTypes::STATE_CHECKED) msaa |= STATE_SYSTEM_CHECKED;
    if(state & AccessibilityTypes::STATE_COLLAPSED) msaa |= STATE_SYSTEM_COLLAPSED;
    if(state & AccessibilityTypes::STATE_DEFAULT) msaa |= STATE_SYSTEM_DEFAULT;
    if(state & AccessibilityTypes::STATE_EXPANDED) msaa |= STATE_SYSTEM_EXPANDED;
    if(state & AccessibilityTypes::STATE_HASPOPUP) msaa |= STATE_SYSTEM_HASPOPUP;
    if(state & AccessibilityTypes::STATE_HOTTRACKED) msaa |= STATE_SYSTEM_HOTTRACKED;
    if(state & AccessibilityTypes::STATE_INVISIBLE) msaa |= STATE_SYSTEM_INVISIBLE;
    if(state & AccessibilityTypes::STATE_LINKED) msaa |= STATE_SYSTEM_LINKED;
    if(state & AccessibilityTypes::STATE_OFFSCREEN) msaa |= STATE_SYSTEM_OFFSCREEN;
    if(state & AccessibilityTypes::STATE_PRESSED) msaa |= STATE_SYSTEM_PRESSED;
    if(state & AccessibilityTypes::STATE_PROTECTED) msaa |= STATE_SYSTEM_PROTECTED;
    if(state & AccessibilityTypes::STATE_READONLY) msaa |= STATE_SYSTEM_READONLY;
    if(state & AccessibilityTypes::STATE_SELECTED) msaa |= STATE_SYSTEM_SELECTED;
    if(state & AccessibilityTypes::STATE_FOCUSED) msaa |= STATE_SYSTEM_FOCUSED;
    if(state & AccessibilityTypes::STATE_UNAVAILABLE) msaa |= STATE_SYSTEM_UNAVAILABLE;
    return msaa;
}
