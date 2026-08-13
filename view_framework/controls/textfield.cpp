
#include "textfield.h"

#include <algorithm>
#include <windows.h>

#include "gfx/canvas.h"

#include "menu/menu_model.h"
#include "menu/menu_runner.h"

#include "../app/event.h"
#include "../app/keyboard_codes_win.h"
#include "../widget/widget.h"

namespace view
{

    namespace
    {
        const int kPadX = 8;
        const int kPadY = 6;
        const int kMinHeight = 28;
        const gfx::Color kSelectionBg(51, 153, 255);
    }

    // static
    const char Textfield::kViewClassName[] = "view/Textfield";

    Textfield::Textfield()
        : style_(STYLE_DEFAULT),
          selection_start_(0),
          cursor_pos_(0),
          read_only_(false),
          context_menu_enabled_(true),
          font_(L"Microsoft YaHei UI", 14),
          text_color_(20, 20, 20),
          background_color_(255, 255, 255),
          border_color_(160, 160, 160),
          controller_(NULL)
    {
        SetFocusable(true);
        SetContextMenuController(this);
    }

    Textfield::Textfield(StyleFlags style)
        : style_(static_cast<int>(style)),
          selection_start_(0),
          cursor_pos_(0),
          read_only_(false),
          context_menu_enabled_(true),
          font_(L"Microsoft YaHei UI", 14),
          text_color_(20, 20, 20),
          background_color_(255, 255, 255),
          border_color_(160, 160, 160),
          controller_(NULL)
    {
        SetFocusable(true);
        SetContextMenuController(this);
    }

    Textfield::~Textfield()
    {
        SetContextMenuController(NULL);
    }

    void Textfield::SetText(const std::wstring& text)
    {
        if(text_ == text)
        {
            return;
        }
        text_ = text;
        cursor_pos_ = text_.size();
        selection_start_ = cursor_pos_;
        SchedulePaint();
        NotifyContentsChanged();
    }

    void Textfield::SelectAll()
    {
        selection_start_ = 0;
        cursor_pos_ = text_.size();
        SchedulePaint();
    }

    void Textfield::ClearSelection()
    {
        selection_start_ = cursor_pos_;
        SchedulePaint();
    }

    bool Textfield::HasSelection() const
    {
        return selection_start_ != cursor_pos_;
    }

    bool Textfield::Cut()
    {
        if(read_only_ || IsPassword())
        {
            return false;
        }
        return CutToClipboard();
    }

    bool Textfield::Copy() const
    {
        return CopyToClipboard();
    }

    bool Textfield::Paste()
    {
        if(read_only_)
        {
            return false;
        }
        return PasteFromClipboard();
    }

    void Textfield::SetContextMenuEnabled(bool enabled)
    {
        context_menu_enabled_ = enabled;
        SetContextMenuController(enabled ? this : NULL);
    }

    void Textfield::SetReadOnly(bool read_only)
    {
        read_only_ = read_only;
        SchedulePaint();
    }

    void Textfield::SetFont(const gfx::Font& font)
    {
        font_ = font;
        SchedulePaint();
    }

    void Textfield::SetTextColor(const gfx::Color& color)
    {
        text_color_ = color;
        SchedulePaint();
    }

    void Textfield::SetBackgroundColor(const gfx::Color& color)
    {
        background_color_ = color;
        SchedulePaint();
    }

    void Textfield::SetBorderColor(const gfx::Color& color)
    {
        border_color_ = color;
        SchedulePaint();
    }

    gfx::Size Textfield::GetPreferredSize()
    {
        const int h = std::max(kMinHeight, font_.GetHeight() + kPadY * 2);
        return gfx::Size(160, h);
    }

    void Textfield::Paint(gfx::Canvas* canvas)
    {
        View::Paint(canvas);

        canvas->FillRectInt(background_color_, 0, 0, width(), height());
        canvas->DrawRectInt(
            HasFocus() ? gfx::Color(40, 110, 200) : border_color_,
            0, 0, width(), height());

        const std::wstring shown = DisplayText();
        const int text_x = kPadX;
        const int text_w = std::max(0, width() - kPadX * 2);
        const int text_h = font_.GetHeight();
        const int text_y = (height() - text_h) / 2;

        size_t sel_a = 0;
        size_t sel_b = 0;
        GetOrderedSelection(&sel_a, &sel_b);
        if(HasFocus() && sel_a < sel_b && sel_b <= shown.size())
        {
            const int x0 = text_x + font_.GetStringWidth(shown.substr(0, sel_a));
            const int x1 = text_x + font_.GetStringWidth(shown.substr(0, sel_b));
            canvas->FillRectInt(kSelectionBg, x0, text_y,
                std::max(1, x1 - x0), text_h);
        }

        canvas->DrawStringInt(shown, font_, text_color_,
            text_x, 0, text_w, height(),
            gfx::Canvas::TEXT_ALIGN_LEFT |
            gfx::Canvas::TEXT_VALIGN_MIDDLE |
            gfx::Canvas::NO_ELLIPSIS);

        if(HasFocus() && !read_only_ && !HasSelection())
        {
            const std::wstring before = shown.substr(0,
                std::min(cursor_pos_, shown.size()));
            const int caret_x = text_x + font_.GetStringWidth(before);
            canvas->FillRectInt(text_color_, caret_x, text_y, 1, text_h);
        }
    }

    bool Textfield::OnMousePressed(const MouseEvent& event)
    {
        if(!event.IsOnlyLeftMouseButton())
        {
            return false;
        }
        RequestFocus();

        if(event.GetFlags() & MouseEvent::EF_IS_DOUBLE_CLICK)
        {
            SelectAll();
            return true;
        }

        const size_t pos = HitTestCursor(event.x());
        SetCursor(pos, event.IsShiftDown());
        return true;
    }

    bool Textfield::OnMouseDragged(const MouseEvent& event)
    {
        if(!event.IsLeftMouseButton())
        {
            return false;
        }
        SetCursor(HitTestCursor(event.x()), true);
        return true;
    }

    bool Textfield::OnKeyPressed(const KeyEvent& event)
    {
        if(controller_ && controller_->HandleKeyEvent(this, event))
        {
            return true;
        }

        const KeyboardCode code = event.GetKeyCode();
        const bool ctrl = event.IsControlDown();
        const bool shift = event.IsShiftDown();

        if(ctrl && code == VKEY_A)
        {
            SelectAll();
            return true;
        }
        if(ctrl && code == VKEY_C)
        {
            Copy();
            return true;
        }
        if(ctrl && code == VKEY_X)
        {
            Cut();
            return true;
        }
        if(ctrl && code == VKEY_V)
        {
            Paste();
            return true;
        }

        if(read_only_)
        {
            return false;
        }

        if(code == VKEY_BACK)
        {
            DeleteSelectionOrChar(false);
            return true;
        }
        if(code == VKEY_DELETE)
        {
            DeleteSelectionOrChar(true);
            return true;
        }
        if(code == VKEY_LEFT)
        {
            if(HasSelection() && !shift)
            {
                size_t a = 0, b = 0;
                GetOrderedSelection(&a, &b);
                SetCursor(a, false);
            }
            else if(cursor_pos_ > 0)
            {
                SetCursor(cursor_pos_ - 1, shift);
            }
            else if(!shift)
            {
                ClearSelection();
            }
            return true;
        }
        if(code == VKEY_RIGHT)
        {
            if(HasSelection() && !shift)
            {
                size_t a = 0, b = 0;
                GetOrderedSelection(&a, &b);
                SetCursor(b, false);
            }
            else if(cursor_pos_ < text_.size())
            {
                SetCursor(cursor_pos_ + 1, shift);
            }
            else if(!shift)
            {
                ClearSelection();
            }
            return true;
        }
        if(code == VKEY_HOME)
        {
            SetCursor(0, shift);
            return true;
        }
        if(code == VKEY_END)
        {
            SetCursor(text_.size(), shift);
            return true;
        }
        if(code == VKEY_RETURN || code == VKEY_TAB || code == VKEY_ESCAPE)
        {
            return false;
        }

        if(ctrl)
        {
            return false;
        }

        wchar_t ch = 0;
        if(MapKeyToChar(event, &ch) && ch >= 32)
        {
            InsertChar(ch);
            return true;
        }
        return false;
    }

    void Textfield::OnFocus()
    {
        SchedulePaint();
    }

    void Textfield::OnBlur()
    {
        SchedulePaint();
    }

    std::string Textfield::GetClassName() const
    {
        return kViewClassName;
    }

    AccessibilityTypes::Role Textfield::GetAccessibleRole()
    {
        return AccessibilityTypes::ROLE_TEXT;
    }

    void Textfield::ShowContextMenu(View* source,
        const gfx::Point& p,
        bool is_mouse_gesture)
    {
        if(!context_menu_enabled_)
        {
            return;
        }

        RequestFocus();

        MenuModel model;
        BuildEditMenuModel(&model);

        Widget* widget = GetWidget();
        if(!widget)
        {
            return;
        }

        const int cmd = MenuRunner::Run(widget->GetNativeView(), model,
            p.x(), p.y());
        switch(cmd)
        {
        case EDIT_CUT:
            Cut();
            break;
        case EDIT_COPY:
            Copy();
            break;
        case EDIT_PASTE:
            Paste();
            break;
        case EDIT_SELECT_ALL:
            SelectAll();
            break;
        default:
            break;
        }
    }

    void Textfield::BuildEditMenuModel(MenuModel* model) const
    {
        if(!model)
        {
            return;
        }
        model->Clear();
        model->AddCommand(EDIT_CUT, L"\u526a\u5207(&T)");
        model->AddCommand(EDIT_COPY, L"\u590d\u5236(&C)");
        model->AddCommand(EDIT_PASTE, L"\u7c98\u8d34(&P)");
        model->AddSeparator();
        model->AddCommand(EDIT_SELECT_ALL, L"\u5168\u9009(&A)");

        const bool has_sel = HasSelection();
        const bool can_copy = has_sel && !IsPassword();
        const bool can_cut = can_copy && !read_only_;
        const bool can_paste = !read_only_ &&
            IsClipboardFormatAvailable(CF_UNICODETEXT);

        model->SetEnabled(EDIT_CUT, can_cut);
        model->SetEnabled(EDIT_COPY, can_copy);
        model->SetEnabled(EDIT_PASTE, can_paste);
        model->SetEnabled(EDIT_SELECT_ALL, !text_.empty());
    }

    std::wstring Textfield::DisplayText() const
    {
        if(!IsPassword())
        {
            return text_;
        }
        return std::wstring(text_.size(), L'\x25CF');
    }

    size_t Textfield::HitTestCursor(int local_x) const
    {
        const std::wstring shown = DisplayText();
        const int x = local_x - kPadX;
        if(x <= 0 || shown.empty())
        {
            return 0;
        }
        size_t pos = 0;
        for(; pos < shown.size(); ++pos)
        {
            const int left = font_.GetStringWidth(shown.substr(0, pos));
            const int right = font_.GetStringWidth(shown.substr(0, pos + 1));
            if(x < (left + right) / 2)
            {
                return pos;
            }
        }
        return shown.size();
    }

    void Textfield::GetOrderedSelection(size_t* start, size_t* end) const
    {
        if(!start || !end)
        {
            return;
        }
        *start = std::min(selection_start_, cursor_pos_);
        *end = std::max(selection_start_, cursor_pos_);
    }

    void Textfield::SetCursor(size_t pos, bool extend_selection)
    {
        if(pos > text_.size())
        {
            pos = text_.size();
        }
        cursor_pos_ = pos;
        if(!extend_selection)
        {
            selection_start_ = cursor_pos_;
        }
        SchedulePaint();
    }

    void Textfield::DeleteSelection()
    {
        if(!HasSelection())
        {
            return;
        }
        size_t a = 0;
        size_t b = 0;
        GetOrderedSelection(&a, &b);
        text_.erase(a, b - a);
        cursor_pos_ = a;
        selection_start_ = a;
        SchedulePaint();
        NotifyContentsChanged();
    }

    void Textfield::InsertChar(wchar_t ch)
    {
        InsertText(std::wstring(1, ch));
    }

    void Textfield::InsertText(const std::wstring& text)
    {
        if(text.empty())
        {
            return;
        }
        if(HasSelection())
        {
            size_t a = 0;
            size_t b = 0;
            GetOrderedSelection(&a, &b);
            text_.erase(a, b - a);
            cursor_pos_ = a;
        }
        text_.insert(cursor_pos_, text);
        cursor_pos_ += text.size();
        selection_start_ = cursor_pos_;
        SchedulePaint();
        NotifyContentsChanged();
    }

    void Textfield::DeleteSelectionOrChar(bool forward)
    {
        if(HasSelection())
        {
            DeleteSelection();
            return;
        }
        if(text_.empty())
        {
            return;
        }
        if(forward)
        {
            if(cursor_pos_ < text_.size())
            {
                text_.erase(cursor_pos_, 1);
                selection_start_ = cursor_pos_;
                SchedulePaint();
                NotifyContentsChanged();
            }
        }
        else
        {
            if(cursor_pos_ > 0)
            {
                --cursor_pos_;
                text_.erase(cursor_pos_, 1);
                selection_start_ = cursor_pos_;
                SchedulePaint();
                NotifyContentsChanged();
            }
        }
    }

    void Textfield::NotifyContentsChanged()
    {
        if(controller_)
        {
            controller_->ContentsChanged(this, text_);
        }
    }

    bool Textfield::MapKeyToChar(const KeyEvent& event, wchar_t* out) const
    {
        if(!out)
        {
            return false;
        }
        BYTE state[256] = {0};
        if(!GetKeyboardState(state))
        {
            return false;
        }
        WCHAR buf[4] = {0};
        const int n = ToUnicode(static_cast<UINT>(event.GetKeyCode()),
            0, state, buf, 4, 0);
        if(n >= 1)
        {
            *out = buf[0];
            return true;
        }
        return false;
    }

    bool Textfield::CopyToClipboard() const
    {
        if(!HasSelection() || IsPassword())
        {
            return false;
        }
        size_t a = 0;
        size_t b = 0;
        GetOrderedSelection(&a, &b);
        const std::wstring selected = text_.substr(a, b - a);
        if(!OpenClipboard(NULL))
        {
            return false;
        }
        EmptyClipboard();
        const size_t bytes = (selected.size() + 1) * sizeof(wchar_t);
        HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if(!mem)
        {
            CloseClipboard();
            return false;
        }
        void* locked = GlobalLock(mem);
        if(!locked)
        {
            GlobalFree(mem);
            CloseClipboard();
            return false;
        }
        memcpy(locked, selected.c_str(), bytes);
        GlobalUnlock(mem);
        const bool ok = SetClipboardData(CF_UNICODETEXT, mem) != NULL;
        if(!ok)
        {
            GlobalFree(mem);
        }
        CloseClipboard();
        return ok;
    }

    bool Textfield::PasteFromClipboard()
    {
        if(!IsClipboardFormatAvailable(CF_UNICODETEXT) || !OpenClipboard(NULL))
        {
            return false;
        }
        HANDLE data = GetClipboardData(CF_UNICODETEXT);
        bool ok = false;
        if(data)
        {
            const wchar_t* text = static_cast<const wchar_t*>(GlobalLock(data));
            if(text)
            {
                std::wstring paste(text);
                GlobalUnlock(data);
                // Single-line: strip CR/LF.
                std::wstring filtered;
                filtered.reserve(paste.size());
                for(size_t i = 0; i < paste.size(); ++i)
                {
                    if(paste[i] != L'\r' && paste[i] != L'\n')
                    {
                        filtered.push_back(paste[i]);
                    }
                }
                InsertText(filtered);
                ok = true;
            }
        }
        CloseClipboard();
        return ok;
    }

    bool Textfield::CutToClipboard()
    {
        if(!CopyToClipboard())
        {
            return false;
        }
        DeleteSelection();
        return true;
    }

} //namespace view
