#pragma once

#include "auralite/ui/node.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace auralite::ui {

class PopupHost;
class Window;

struct MenuCommand {
  std::wstring text;
  std::function<void()> on_click;
  bool separator = false;
  bool checkable = false;
  bool checked = false;
  int radio_group = 0;
  std::wstring icon;
};

// Horizontal menu titles. Each title opens a PopupHost list of commands.
class MenuBar : public Node {
 public:
  using CommandHandler =
      std::function<void(const std::wstring& menu, const std::wstring& item)>;

  MenuBar();
  ~MenuBar() override;

  void BindWindow(Window* window);

  MenuBar& add_menu(std::wstring title, std::vector<MenuCommand> commands);
  int menu_count() const { return static_cast<int>(menus_.size()); }
  const std::wstring& menu_title(int index) const;
  MenuBar& on_command(CommandHandler handler);

  // Dropdown HWND chrome (layered popup). 0 radius = square; border uses theme.border.
  MenuBar& corner_radius(float r);
  float corner_radius() const { return corner_radius_; }
  MenuBar& border_width(float w);
  float border_width() const { return border_width_; }

  AccRole acc_role() const override;
  std::wstring AccDefaultName() const override;

  SizeF Measure(float max_w, float max_h) override;
  void Layout(const RectF& final_rect) override;
  void Paint(auralite::Canvas& canvas) override;

  void OnMouseMove(const MouseEvent& e) override;
  void OnMouseLeave(const MouseEvent& e) override;
  void OnMouseDown(const MouseEvent& e) override;

 private:
  struct Menu {
    std::wstring title;
    std::vector<MenuCommand> commands;
    float x = 0.f;
    float w = 0.f;
  };

  int IndexAt(float x, float y) const;
  void OpenMenu(int index);
  void RebuildLayout(float width);
  POINT ItemScreenPoint(int index) const;

  Window* window_ = nullptr;
  std::unique_ptr<PopupHost> host_;
  std::vector<Menu> menus_;
  CommandHandler on_command_;
  float corner_radius_ = 8.f;
  float border_width_ = 1.f;
  int hover_ = -1;
  int open_ = -1;
};

}  // namespace auralite::ui
