
#include "simple_menu_model_controller.h"

#include "menu_runner.h"

#include "../../widget/widget.h"

namespace view
{

    SimpleMenuModelController::SimpleMenuModelController()
        : delegate_(NULL) {}

    SimpleMenuModelController::SimpleMenuModelController(Delegate* delegate)
        : delegate_(delegate) {}

    SimpleMenuModelController::~SimpleMenuModelController() {}

    void SimpleMenuModelController::ShowContextMenu(View* source,
        const gfx::Point& p,
        bool is_mouse_gesture)
    {
        if(!source || !delegate_)
        {
            return;
        }

        delegate_->UpdateMenuModel(&model_);

        Widget* widget = source->GetWidget();
        if(!widget)
        {
            return;
        }

        const int cmd = MenuRunner::Run(widget->GetNativeView(), model_,
            p.x(), p.y());
        if(cmd != 0)
        {
            delegate_->ExecuteCommand(cmd);
        }
    }

} //namespace view
