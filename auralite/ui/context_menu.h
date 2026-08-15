#pragma once

#include <functional>
#include <string>
#include <vector>
#include <windows.h>

namespace auralite::ui {

// Win32 TrackPopupMenu wrapper (system-menu exception for phase 2).
class ContextMenu {
 public:
  using CommandHandler = std::function<void(int id)>;

  ContextMenu& AddItem(int id, const std::wstring& label);
  ContextMenu& AddSeparator();
  ContextMenu& on_command(CommandHandler handler);

  // |pt| is screen coordinates. Invokes on_command if the user picks an item.
  void Show(HWND owner, POINT pt) const;
  void Show(HWND owner, int screen_x, int screen_y) const;

 private:
  enum class EntryType { Item, Separator };

  struct Entry {
    EntryType type = EntryType::Item;
    int id = 0;
    std::wstring label;
  };

  std::vector<Entry> entries_;
  CommandHandler on_command_;
};

}  // namespace auralite::ui
