#ifndef __menu_model_h__
#define __menu_model_h__

#pragma once

#include <string>
#include <vector>

#include "base/basic_types.h"

namespace view
{

    // Snapshot of a flat popup menu (commands + separators).
    class MenuModel
    {
    public:
        enum ItemType
        {
            TYPE_COMMAND,
            TYPE_SEPARATOR
        };

        struct Item
        {
            ItemType type;
            int command_id;
            std::wstring label;
            bool enabled;
            bool checked;

            Item()
                : type(TYPE_COMMAND),
                  command_id(0),
                  enabled(true),
                  checked(false) {}
        };

        MenuModel();
        ~MenuModel();

        void Clear();
        void AddCommand(int command_id, const std::wstring& label);
        void AddSeparator();
        void SetEnabled(int command_id, bool enabled);
        void SetChecked(int command_id, bool checked);

        size_t GetItemCount() const { return items_.size(); }
        const Item& GetItemAt(size_t index) const;

    private:
        Item* FindCommand(int command_id);

        std::vector<Item> items_;

        DISALLOW_COPY_AND_ASSIGN(MenuModel);
    };

} //namespace view

#endif //__menu_model_h__
