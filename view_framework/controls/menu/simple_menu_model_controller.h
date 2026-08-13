#ifndef __simple_menu_model_controller_h__
#define __simple_menu_model_controller_h__

#pragma once

#include "menu_model.h"
#include "../../view.h"

namespace view
{

    // ContextMenuController that shows a MenuModel and dispatches commands.
    class SimpleMenuModelController : public ContextMenuController
    {
    public:
        class Delegate
        {
        public:
            virtual void ExecuteCommand(int command_id) = 0;
            // Optional: refresh enable/check state before showing.
            virtual void UpdateMenuModel(MenuModel* model) {}

        protected:
            virtual ~Delegate() {}
        };

        SimpleMenuModelController();
        explicit SimpleMenuModelController(Delegate* delegate);
        virtual ~SimpleMenuModelController();

        MenuModel* model() { return &model_; }
        const MenuModel* model() const { return &model_; }

        void SetDelegate(Delegate* delegate) { delegate_ = delegate; }

        // Overridden from ContextMenuController:
        virtual void ShowContextMenu(View* source,
            const gfx::Point& p,
            bool is_mouse_gesture);

    private:
        MenuModel model_;
        Delegate* delegate_;

        DISALLOW_COPY_AND_ASSIGN(SimpleMenuModelController);
    };

} //namespace view

#endif //__simple_menu_model_controller_h__
