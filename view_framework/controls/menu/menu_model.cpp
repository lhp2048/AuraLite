
#include "menu_model.h"

namespace view
{

    MenuModel::MenuModel() {}

    MenuModel::~MenuModel() {}

    void MenuModel::Clear()
    {
        items_.clear();
    }

    void MenuModel::AddCommand(int command_id, const std::wstring& label)
    {
        Item item;
        item.type = TYPE_COMMAND;
        item.command_id = command_id;
        item.label = label;
        item.enabled = true;
        item.checked = false;
        items_.push_back(item);
    }

    void MenuModel::AddSeparator()
    {
        Item item;
        item.type = TYPE_SEPARATOR;
        item.command_id = 0;
        items_.push_back(item);
    }

    void MenuModel::SetEnabled(int command_id, bool enabled)
    {
        Item* item = FindCommand(command_id);
        if(item)
        {
            item->enabled = enabled;
        }
    }

    void MenuModel::SetChecked(int command_id, bool checked)
    {
        Item* item = FindCommand(command_id);
        if(item)
        {
            item->checked = checked;
        }
    }

    const MenuModel::Item& MenuModel::GetItemAt(size_t index) const
    {
        return items_[index];
    }

    MenuModel::Item* MenuModel::FindCommand(int command_id)
    {
        for(size_t i = 0; i < items_.size(); ++i)
        {
            if(items_[i].type == TYPE_COMMAND &&
                items_[i].command_id == command_id)
            {
                return &items_[i];
            }
        }
        return NULL;
    }

} //namespace view
