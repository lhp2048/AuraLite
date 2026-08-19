#include "mx/ui/context_menu.h"

namespace mx::ui {

ContextMenu& ContextMenu::AddItem(int id, const std::wstring& label) {
  Entry e;
  e.type = EntryType::Item;
  e.id = id;
  e.label = label;
  entries_.push_back(std::move(e));
  return *this;
}

ContextMenu& ContextMenu::AddSeparator() {
  Entry e;
  e.type = EntryType::Separator;
  entries_.push_back(std::move(e));
  return *this;
}

ContextMenu& ContextMenu::on_command(CommandHandler handler) {
  on_command_ = std::move(handler);
  return *this;
}

void ContextMenu::Show(HWND owner, int screen_x, int screen_y) const {
  POINT pt = {screen_x, screen_y};
  Show(owner, pt);
}

void ContextMenu::Show(HWND owner, POINT pt) const {
  if (!owner || entries_.empty()) {
    return;
  }

  HMENU menu = CreatePopupMenu();
  if (!menu) {
    return;
  }

  for (const Entry& e : entries_) {
    if (e.type == EntryType::Separator) {
      AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
      continue;
    }
    AppendMenuW(menu, MF_STRING, static_cast<UINT_PTR>(e.id), e.label.c_str());
  }

  const UINT flags =
      TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY;
  const int cmd = static_cast<int>(
      TrackPopupMenu(menu, flags, pt.x, pt.y, 0, owner, nullptr));
  DestroyMenu(menu);

  if (cmd != 0 && on_command_) {
    on_command_(cmd);
  }
}

}  // namespace mx::ui
