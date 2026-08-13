#ifndef __menu_runner_h__
#define __menu_runner_h__

#pragma once

#include <windows.h>

#include "base/basic_types.h"

namespace view
{

    class MenuModel;

    // Runs a flat Win32 popup menu from a MenuModel.
    class MenuRunner
    {
    public:
        // Shows a popup at screen coordinates. Returns selected command_id,
        // or 0 if the user cancels.
        static int Run(HWND owner,
            const MenuModel& model,
            int screen_x,
            int screen_y);

    private:
        MenuRunner();
        DISALLOW_COPY_AND_ASSIGN(MenuRunner);
    };

} //namespace view

#endif //__menu_runner_h__
