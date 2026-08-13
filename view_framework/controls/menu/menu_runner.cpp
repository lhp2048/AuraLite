
#include "menu_runner.h"

#include "menu_model.h"

#include "base/basic_types.h"
#include "base/logging.h"

namespace view
{

    // static
    int MenuRunner::Run(HWND owner,
        const MenuModel& model,
        int screen_x,
        int screen_y)
    {
        HMENU menu = CreatePopupMenu();
        if(!menu)
        {
            return 0;
        }

        for(size_t i = 0; i < model.GetItemCount(); ++i)
        {
            const MenuModel::Item& item = model.GetItemAt(i);
            if(item.type == MenuModel::TYPE_SEPARATOR)
            {
                AppendMenu(menu, MF_SEPARATOR, 0, NULL);
                continue;
            }

            UINT flags = MF_STRING;
            if(!item.enabled)
            {
                flags |= MF_GRAYED;
            }
            if(item.checked)
            {
                flags |= MF_CHECKED;
            }
            AppendMenu(menu, flags,
                static_cast<UINT_PTR>(item.command_id),
                item.label.c_str());
        }

        // Align to the clicked point; avoid covering the cursor awkwardly.
        const UINT tpm_flags = TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON |
            TPM_RETURNCMD | TPM_NONOTIFY;
        const int cmd = static_cast<int>(TrackPopupMenu(menu, tpm_flags,
            screen_x, screen_y, 0, owner, NULL));
        DestroyMenu(menu);
        return cmd;
    }

} //namespace view
