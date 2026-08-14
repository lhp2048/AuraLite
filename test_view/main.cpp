#include <tchar.h>
#include <windows.h>
#include <initguid.h>
#include <oleacc.h>
#include <stdio.h>

#include "gfx/gdiplus_initializer.h"
#include "view_framework/animation/bounds_animator.h"
#include "view_framework/app/resource_bundle.h"
#include "view_framework/controls/button/image_button.h"
#include "view_framework/controls/button/text_button.h"
#include "view_framework/controls/checkbox.h"
#include "view_framework/controls/image_view.h"
#include "view_framework/controls/label.h"
#include "view_framework/controls/menu/simple_menu_model_controller.h"
#include "view_framework/controls/radio_button.h"
#include "view_framework/controls/scroll_view.h"
#include "view_framework/controls/switch.h"
#include "view_framework/controls/textfield.h"
#include "view_framework/controls/single_split_view.h"
#include "view_framework/focus/accelerator_handler.h"
#include "view_framework/window/dialog_delegate.h"
#include "view_framework/window/window_win.h"
#include "view_framework/widget/root_view.h"
#include "view_framework/layout/box_layout.h"
#include "view_framework/layout/grid_layout.h"

#include "resource.h"

#define BUTTON_ID_ANIMATE   1

class MainView : public view::View
{
public:
    virtual gfx::Size GetPreferredSize() { return gfx::Size(200, 200); }
};

class MainWindowDelegate : public view::WindowDelegate,
    public view::ButtonListener,
    public view::SimpleMenuModelController::Delegate
{
    MainView* content_view_;
    scoped_ptr<view::BoundsAnimator> animator_;
    scoped_ptr<view::SimpleMenuModelController> demo_menu_;

public:
    MainWindowDelegate()
    {
        content_view_ = new MainView();
        view::GridLayout* layout = new view::GridLayout(content_view_);
        content_view_->set_background(
            view::Background::CreateSolidBackground(gfx::Color(214, 229, 247)));
        content_view_->SetAccessibleName(L"内容视图");
        content_view_->SetLayoutManager(layout);

        view::ColumnSet* column_set = layout->AddColumnSet(0);
        column_set->AddColumn(view::GridLayout::FILL, view::GridLayout::FILL,
            1, view::GridLayout::USE_PREF, 0, 0);

        const gfx::Font ui_font(L"Microsoft YaHei UI", 16);

        layout->StartRow(0, 0);
        view::Label* title = new view::Label(
            L"基础控件：Label / Textfield / Menu / Radio / Switch / Image / Scroll");
        title->SetFont(ui_font);
        title->SetColor(gfx::Color(30, 60, 120));
        layout->AddView(title);

        layout->StartRow(0, 0);
        view::Textfield* field = new view::Textfield();
        field->SetFont(ui_font);
        field->SetText(L"可编辑文本");
        layout->AddView(field);

        layout->StartRow(0, 0);
        view::Textfield* pwd = new view::Textfield(view::Textfield::STYLE_PASSWORD);
        pwd->SetFont(ui_font);
        pwd->SetText(L"密文密码");
        layout->AddView(pwd);

        layout->StartRow(0, 0);
        view::Checkbox* check = new view::Checkbox(L"启用示例选项");
        check->SetFont(ui_font);
        layout->AddView(check);

        layout->StartRow(0, 0);
        view::Label* menu_tip = new view::Label(L"右键此处测试通用菜单");
        menu_tip->SetFont(ui_font);
        demo_menu_.reset(new view::SimpleMenuModelController(this));
        demo_menu_->model()->AddCommand(1001, L"示例项一");
        demo_menu_->model()->AddCommand(1002, L"示例项二");
        demo_menu_->model()->AddSeparator();
        demo_menu_->model()->AddCommand(1003, L"关闭菜单测试");
        menu_tip->SetContextMenuController(demo_menu_.get());
        layout->AddView(menu_tip);

        layout->StartRow(0, 0);
        view::View* radio_row = new view::View();
        radio_row->SetLayoutManager(new view::BoxLayout(
            view::BoxLayout::kHorizontal, 0, 0, 16));
        view::RadioButton* radio1 = new view::RadioButton(L"选项 A", 1);
        radio1->SetFont(ui_font);
        radio1->SetChecked(true);
        radio_row->AddChildView(radio1);
        view::RadioButton* radio2 = new view::RadioButton(L"选项 B", 1);
        radio2->SetFont(ui_font);
        radio_row->AddChildView(radio2);
        layout->AddView(radio_row);

        layout->StartRow(0, 0);
        view::Switch* sw = new view::Switch(L"示例开关");
        sw->SetFont(ui_font);
        layout->AddView(sw);

        layout->StartRow(0, 0);
        view::View* image_row = new view::View();
        image_row->SetLayoutManager(new view::BoxLayout(
            view::BoxLayout::kHorizontal, 0, 0, 8));
        view::ImageView* image = new view::ImageView();
        image->SetImage(ResourceBundle::GetSharedInstance().GetBitmapNamed(
            IDR_DEFAULT_FAVICON));
        image_row->AddChildView(image);
        view::ImageButton* image_button = new view::ImageButton(NULL);
        image_button->SetImage(view::CustomButton::BS_NORMAL,
            ResourceBundle::GetSharedInstance().GetBitmapNamed(
            IDR_DEFAULT_FAVICON));
        image_row->AddChildView(image_button);
        view::Label* image_label = new view::Label(L"ImageView");
        image_label->SetFont(ui_font);
        image_row->AddChildView(image_label);
        layout->AddView(image_row);

        layout->StartRow(0, 0);
        view::ScrollView* scroll = new view::ScrollView();
        view::View* scroll_content = new view::View();
        scroll_content->SetLayoutManager(new view::BoxLayout(
            view::BoxLayout::kVertical, 4, 4, 4));
        for(int i = 1; i <= 12; ++i)
        {
            wchar_t buf[64] = {0};
            _snwprintf_s(buf, _TRUNCATE, L"滚动列表项 %d", i);
            view::Label* item = new view::Label(buf);
            item->SetFont(ui_font);
            scroll_content->AddChildView(item);
        }
        scroll->SetContents(scroll_content);
        layout->AddView(scroll, 1, 1,
            view::GridLayout::FILL, view::GridLayout::FILL, 280, 120);

        layout->StartRow(0, 0);
        view::TextButton* button = new view::TextButton(NULL, L"带图标按钮");
        button->SetFocusable(true);
        button->SetFont(ui_font);
        button->SetIcon(ResourceBundle::GetSharedInstance().GetBitmapNamed(
            IDR_DEFAULT_FAVICON));
        layout->AddView(button);

        layout->StartRow(0, 0);
        button = new view::TextButton(NULL, L"文本居中对齐按钮");
        button->SetFocusable(true);
        button->SetFont(ui_font);
        button->set_alignment(view::TextButton::ALIGN_CENTER);
        layout->AddView(button);

        layout->StartRow(0, 0);
        button = new view::TextButton(NULL, L"文本右对齐按钮");
        button->SetFocusable(true);
        button->SetFont(ui_font);
        button->set_alignment(view::TextButton::ALIGN_RIGHT);
        layout->AddView(button);

        layout->StartRow(0, 0);
        view::View* v = new view::View();
        layout->AddView(v);
        v->SetLayoutManager(new view::BoxLayout(
            view::BoxLayout::kHorizontal, 0, 0, 0));
        button = new view::TextButton(NULL, L"按钮一");
        button->SetFocusable(true);
        button->SetFont(ui_font);
        v->AddChildView(button);
        button = new view::TextButton(NULL, L"按钮二");
        button->SetFocusable(true);
        button->SetFont(ui_font);
        v->AddChildView(button);
        button = new view::TextButton(NULL, L"按钮三");
        button->SetFocusable(true);
        button->SetFont(ui_font);
        v->AddChildView(button);

        layout->StartRow(1, 0);
        view::View* v1 = new view::View();
        view::View* v2 = new view::View();
        v1->set_background(view::Background::CreateStandardPanelBackground());
        v1->set_border(view::Border::CreateSolidBorder(1, gfx::Color(80, 80, 80)));
        v2->set_border(view::Border::CreateSolidBorder(1, gfx::Color(80, 80, 80)));
        v2->set_background(view::Background::CreateStandardPanelBackground());
        layout->AddView(new view::SingleSplitView(v1, v2,
            view::SingleSplitView::HORIZONTAL_SPLIT));

        layout->StartRow(1, 0);
        v = new view::View();
        layout->AddView(v);
        button = new view::TextButton(this, L"动画按钮");
        button->SetID(BUTTON_ID_ANIMATE);
        button->SetFocusable(true);
        button->SetFont(ui_font);
        button->SetIcon(ResourceBundle::GetSharedInstance().GetBitmapNamed(
            IDR_DEFAULT_FAVICON));
        button->set_alignment(view::TextButton::ALIGN_CENTER);
        button->SetBounds(100, 20, 100, 30);
        v->AddChildView(button);
    }

    ~MainWindowDelegate() {}

    virtual bool CanResize() const
    {
        return true;
    }

    virtual bool CanMaximize() const
    {
        return false;
    }

    virtual std::wstring GetWindowTitle() const
    {
        return L"测试视图";
    }

    virtual void WindowClosing()
    {
        MessageLoopForUI::current()->Quit();
    }

    virtual view::View* GetContentsView()
    {
        return content_view_;
    }

    virtual void ExecuteCommand(int command_id)
    {
        if(content_view_)
        {
            wchar_t buf[64] = {0};
            _snwprintf_s(buf, _TRUNCATE, L"菜单命令 %d", command_id);
            content_view_->SetAccessibleName(buf);
        }
    }

    virtual void ButtonPressed(view::Button* sender, const view::Event& event)
    {
        if(sender->GetID()==BUTTON_ID_ANIMATE)
        {
            if(!animator_.get())
            {
                animator_.reset(new view::BoundsAnimator(sender->GetParent()));
            }

            if(animator_->IsAnimating())
            {
                return ;
            }

            gfx::Rect new_pos = sender->bounds();
            if(new_pos.x() == 100)
            {
                new_pos.Offset(100, 100);
                new_pos.set_width(new_pos.width()*2);
                new_pos.set_height(new_pos.height()*2);
            }
            else
            {
                new_pos.Offset(-100, -100);
                new_pos.set_width(new_pos.width()/2);
                new_pos.set_height(new_pos.height()/2);
            }

            DCHECK(animator_.get());
            animator_->AnimateViewTo(sender, new_pos);
        }
    }
};

class AppIdConveter : public ResourceBundle::IdConveter
{
public:
    int AppIdToResId(ResourceBundle::AppId app_id)
    {
        int resource_id = 0;

        switch(app_id)
        {
        case ResourceBundle::BITMAP_FRAME:
            resource_id = IDR_FRAME;
            break;
        case ResourceBundle::BITMAP_FRAME_INACTIVE:
            resource_id = IDR_FRAME_INACTIVE;
            break;
        case ResourceBundle::BITMAP_WINDOW_TOP_LEFT_CORNER:
            resource_id = IDR_WINDOW_TOP_LEFT_CORNER;
            break;
        case ResourceBundle::BITMAP_WINDOW_TOP_CENTER:
            resource_id = IDR_WINDOW_TOP_CENTER;
            break;
        case ResourceBundle::BITMAP_WINDOW_TOP_RIGHT_CORNER:
            resource_id = IDR_WINDOW_TOP_RIGHT_CORNER;
            break;
        case ResourceBundle::BITMAP_WINDOW_LEFT_SIDE:
            resource_id = IDR_WINDOW_LEFT_SIDE;
            break;
        case ResourceBundle::BITMAP_WINDOW_RIGHT_SIDE:
            resource_id = IDR_WINDOW_RIGHT_SIDE;
            break;
        case ResourceBundle::BITMAP_WINDOW_BOTTOM_LEFT_CORNER:
            resource_id = IDR_WINDOW_BOTTOM_LEFT_CORNER;
            break;
        case ResourceBundle::BITMAP_WINDOW_BOTTOM_CENTER:
            resource_id = IDR_WINDOW_BOTTOM_CENTER;
            break;
        case ResourceBundle::BITMAP_WINDOW_BOTTOM_RIGHT_CORNER:
            resource_id = IDR_WINDOW_BOTTOM_RIGHT_CORNER;
            break;
        case ResourceBundle::BITMAP_APP_TOP_LEFT:
            resource_id = IDR_APP_TOP_LEFT;
            break;
        case ResourceBundle::BITMAP_APP_TOP_CENTER:
            resource_id = IDR_APP_TOP_CENTER;
            break;
        case ResourceBundle::BITMAP_APP_TOP_RIGHT:
            resource_id = IDR_APP_TOP_RIGHT;
            break;
        case ResourceBundle::BITMAP_CONTENT_BOTTOM_LEFT_CORNER:
            resource_id = IDR_CONTENT_BOTTOM_LEFT_CORNER;
            break;
        case ResourceBundle::BITMAP_CONTENT_BOTTOM_CENTER:
            resource_id = IDR_CONTENT_BOTTOM_CENTER;
            break;
        case ResourceBundle::BITMAP_CONTENT_BOTTOM_RIGHT_CORNER:
            resource_id = IDR_CONTENT_BOTTOM_RIGHT_CORNER;
            break;
        case ResourceBundle::BITMAP_CONTENT_LEFT_SIDE:
            resource_id = IDR_CONTENT_LEFT_SIDE;
            break;
        case ResourceBundle::BITMAP_CONTENT_RIGHT_SIDE:
            resource_id = IDR_CONTENT_RIGHT_SIDE;
            break;
        case ResourceBundle::BITMAP_CLOSE:
            resource_id = IDR_CLOSE;
            break;
        case ResourceBundle::BITMAP_CLOSE_SA:
            resource_id = IDR_CLOSE_SA;
            break;
        case ResourceBundle::BITMAP_CLOSE_H:
            resource_id = IDR_CLOSE_H;
            break;
        case ResourceBundle::BITMAP_CLOSE_SA_H:
            resource_id = IDR_CLOSE_SA_H;
            break;
        case ResourceBundle::BITMAP_CLOSE_P:
            resource_id = IDR_CLOSE_P;
            break;
        case ResourceBundle::BITMAP_CLOSE_SA_P:
            resource_id = IDR_CLOSE_SA_P;
            break;
        case ResourceBundle::BITMAP_RESTORE:
            resource_id = IDR_RESTORE;
            break;
        case ResourceBundle::BITMAP_RESTORE_H:
            resource_id = IDR_RESTORE_H;
            break;
        case ResourceBundle::BITMAP_RESTORE_P:
            resource_id = IDR_RESTORE_P;
            break;
        case ResourceBundle::BITMAP_MAXIMIZE:
            resource_id = IDR_MAXIMIZE;
            break;
        case ResourceBundle::BITMAP_MAXIMIZE_H:
            resource_id = IDR_MAXIMIZE_H;
            break;
        case ResourceBundle::BITMAP_MAXIMIZE_P:
            resource_id = IDR_MAXIMIZE_P;
            break;
        case ResourceBundle::BITMAP_MINIMIZE:
            resource_id = IDR_MINIMIZE;
            break;
        case ResourceBundle::BITMAP_MINIMIZE_H:
            resource_id = IDR_MINIMIZE_H;
            break;
        case ResourceBundle::BITMAP_MINIMIZE_P:
            resource_id = IDR_MINIMIZE_P;
            break;

        case ResourceBundle::BITMAP_TEXTBUTTON_TOP_LEFT_H:
            resource_id = IDR_TEXTBUTTON_TOP_LEFT_H;
            break;
        case ResourceBundle::BITMAP_TEXTBUTTON_TOP_H:
            resource_id = IDR_TEXTBUTTON_TOP_H;
            break;
        case ResourceBundle::BITMAP_TEXTBUTTON_TOP_RIGHT_H:
            resource_id = IDR_TEXTBUTTON_TOP_RIGHT_H;
            break;
        case ResourceBundle::BITMAP_TEXTBUTTON_LEFT_H:
            resource_id = IDR_TEXTBUTTON_LEFT_H;
            break;
        case ResourceBundle::BITMAP_TEXTBUTTON_CENTER_H:
            resource_id = IDR_TEXTBUTTON_CENTER_H;
            break;
        case ResourceBundle::BITMAP_TEXTBUTTON_RIGHT_H:
            resource_id = IDR_TEXTBUTTON_RIGHT_H;
            break;
        case ResourceBundle::BITMAP_TEXTBUTTON_BOTTOM_LEFT_H:
            resource_id = IDR_TEXTBUTTON_BOTTOM_LEFT_H;
            break;
        case ResourceBundle::BITMAP_TEXTBUTTON_BOTTOM_H:
            resource_id = IDR_TEXTBUTTON_BOTTOM_H;
            break;
        case ResourceBundle::BITMAP_TEXTBUTTON_BOTTOM_RIGHT_H:
            resource_id = IDR_TEXTBUTTON_BOTTOM_RIGHT_H;
            break;
        case ResourceBundle::BITMAP_TEXTBUTTON_TOP_LEFT_P:
            resource_id = IDR_TEXTBUTTON_TOP_LEFT_P;
            break;
        case ResourceBundle::BITMAP_TEXTBUTTON_TOP_P:
            resource_id = IDR_TEXTBUTTON_TOP_P;
            break;
        case ResourceBundle::BITMAP_TEXTBUTTON_TOP_RIGHT_P:
            resource_id = IDR_TEXTBUTTON_TOP_RIGHT_P;
            break;
        case ResourceBundle::BITMAP_TEXTBUTTON_LEFT_P:
            resource_id = IDR_TEXTBUTTON_LEFT_P;
            break;
        case ResourceBundle::BITMAP_TEXTBUTTON_CENTER_P:
            resource_id = IDR_TEXTBUTTON_CENTER_P;
            break;
        case ResourceBundle::BITMAP_TEXTBUTTON_RIGHT_P:
            resource_id = IDR_TEXTBUTTON_RIGHT_P;
            break;
        case ResourceBundle::BITMAP_TEXTBUTTON_BOTTOM_LEFT_P:
            resource_id = IDR_TEXTBUTTON_BOTTOM_LEFT_P;
            break;
        case ResourceBundle::BITMAP_TEXTBUTTON_BOTTOM_P:
            resource_id = IDR_TEXTBUTTON_BOTTOM_P;
            break;
        case ResourceBundle::BITMAP_TEXTBUTTON_BOTTOM_RIGHT_P:
            resource_id = IDR_TEXTBUTTON_BOTTOM_RIGHT_P;
            break;
        }

        return resource_id;
    }
};

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    HRESULT hRes = OleInitialize(NULL);

    // this resolves ATL window thunking problem when Microsoft Layer for Unicode (MSLU) is used
    ::DefWindowProc(NULL, 0, 0, 0L);

    base::AtExitManager exit_manager;

    gfx::GdiplusInitializer gdiplus_initializer;
    gdiplus_initializer.Init();
    ResourceBundle::InitSharedInstance(base::FilePath());
    ResourceBundle::GetSharedInstance().SetIdConveter(
        new AppIdConveter());
    view::AcceleratorHandler handler;
    MessageLoop loop(MessageLoop::TYPE_UI);

    MainWindowDelegate delegate;
    view::Window::CreateWanWindow(NULL, gfx::Rect(), &delegate);
    delegate.window()->SetWindowBounds(gfx::Rect(0, 0, 500, 500), NULL);
    delegate.window()->Show();

    MessageLoopForUI::current()->Run(&handler);

    ResourceBundle::CleanupSharedInstance();

    gdiplus_initializer.UnInit();

    OleUninitialize();
}