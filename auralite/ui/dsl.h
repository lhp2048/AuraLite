#pragma once

#include "auralite/ui/button.h"
#include "auralite/ui/checkbox.h"
#include "auralite/ui/column.h"
#include "auralite/ui/absolute.h"
#include "auralite/ui/combo.h"
#include "auralite/ui/image_button.h"
#include "auralite/ui/image_view.h"
#include "auralite/ui/label.h"
#include "auralite/ui/list_view.h"
#include "auralite/ui/item_list.h"
#include "auralite/ui/virtual_list.h"
#include "auralite/ui/tree_view.h"
#include "auralite/ui/progress_bar.h"
#include "auralite/ui/radio.h"
#include "auralite/ui/row.h"
#include "auralite/ui/scroll_view.h"
#include "auralite/ui/slider.h"
#include "auralite/ui/split_view.h"
#include "auralite/ui/submenu.h"
#include "auralite/ui/switch_control.h"
#include "auralite/ui/tab.h"
#include "auralite/ui/text_area.h"
#include "auralite/ui/text_field.h"
#include "auralite/ui/tile.h"
#include "auralite/ui/user_control.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace auralite::ui::dsl {

namespace detail {

template <typename T>
class BuilderBase {
 public:
  BuilderBase() : node_(std::make_unique<T>()) {}

  std::unique_ptr<Node> Build() { return std::move(node_); }

  T* get() { return node_.get(); }
  const T* get() const { return node_.get(); }

 protected:
  std::unique_ptr<T> node_;
};

template <typename Self, typename T>
class ChildHost : public BuilderBase<T> {
 public:
  Self& bg(const ColorF& c) {
    this->get()->bg(c);
    return static_cast<Self&>(*this);
  }

  Self& child(std::unique_ptr<Node> n) {
    this->node_->AddChild(std::move(n));
    return static_cast<Self&>(*this);
  }

  template <typename B>
  Self& child(B&& builder) {
    return child(std::forward<B>(builder).Build());
  }
};

}  // namespace detail

class ColumnBuilder : public detail::ChildHost<ColumnBuilder, Column> {
 public:
  ColumnBuilder& padding(float all) {
    get()->padding(all);
    return *this;
  }
  ColumnBuilder& padding(float l, float t, float r, float b) {
    get()->padding(l, t, r, b);
    return *this;
  }
  ColumnBuilder& spacing(float s) {
    get()->spacing(s);
    return *this;
  }
  ColumnBuilder& child_align(Align a) {
    get()->child_align(a);
    return *this;
  }
  ColumnBuilder& main_align(Align a) {
    get()->main_align(a);
    return *this;
  }
  ColumnBuilder& fill_width() {
    get()->fill_width();
    return *this;
  }
  ColumnBuilder& fixed_height(float h) {
    get()->fixed_height(h);
    return *this;
  }
};

class RowBuilder : public detail::ChildHost<RowBuilder, Row> {
 public:
  RowBuilder& padding(float all) {
    get()->padding(all);
    return *this;
  }
  RowBuilder& padding(float l, float t, float r, float b) {
    get()->padding(l, t, r, b);
    return *this;
  }
  RowBuilder& spacing(float s) {
    get()->spacing(s);
    return *this;
  }
  RowBuilder& child_align(Align a) {
    get()->child_align(a);
    return *this;
  }
  RowBuilder& main_align(Align a) {
    get()->main_align(a);
    return *this;
  }
  RowBuilder& fill_width() {
    get()->fill_width();
    return *this;
  }
  RowBuilder& fixed_height(float h) {
    get()->fixed_height(h);
    return *this;
  }
};

class TileBuilder : public detail::ChildHost<TileBuilder, Tile> {
 public:
  TileBuilder& padding(float all) {
    get()->padding(all);
    return *this;
  }
  TileBuilder& spacing(float s) {
    get()->spacing(s);
    return *this;
  }
  TileBuilder& spacing(float h, float v) {
    get()->spacing(h, v);
    return *this;
  }
  TileBuilder& columns(int cols) {
    get()->columns(cols);
    return *this;
  }
  TileBuilder& item_size(float w, float h) {
    get()->item_size(w, h);
    return *this;
  }
  TileBuilder& fill_width() {
    get()->fill_width();
    return *this;
  }
  TileBuilder& fixed_height(float h) {
    get()->fixed_height(h);
    return *this;
  }
};

class TabBuilder : public detail::ChildHost<TabBuilder, Tab> {
 public:
  TabBuilder& selected(int index) {
    get()->set_selected(index);
    return *this;
  }
  TabBuilder& headers(std::vector<std::wstring> titles) {
    get()->set_headers(std::move(titles));
    return *this;
  }
  TabBuilder& header_height(float h) {
    get()->header_height(h);
    return *this;
  }
  TabBuilder& on_selected(Tab::SelectedHandler handler) {
    get()->on_selected(std::move(handler));
    return *this;
  }
  TabBuilder& fill_width() {
    get()->fill_width();
    return *this;
  }
  TabBuilder& fixed_height(float h) {
    get()->fixed_height(h);
    return *this;
  }
};

class AbsoluteBuilder : public detail::ChildHost<AbsoluteBuilder, Absolute> {
 public:
  AbsoluteBuilder& fill_width() {
    get()->fill_width();
    return *this;
  }
  AbsoluteBuilder& fixed_height(float h) {
    get()->fixed_height(h);
    return *this;
  }
};

class LabelBuilder : public detail::BuilderBase<Label> {
 public:
  LabelBuilder& text(const std::wstring& t) {
    get()->text(t);
    return *this;
  }
  LabelBuilder& font_size(float size) {
    get()->font_size(size);
    return *this;
  }
  LabelBuilder& color(const ColorF& c) {
    get()->color(c);
    return *this;
  }
  LabelBuilder& align(TextAlign a) {
    get()->align(a);
    return *this;
  }
  LabelBuilder& cross_align(Align a) {
    get()->cross_align(a);
    return *this;
  }
  LabelBuilder& weight(float w) {
    get()->weight(w);
    return *this;
  }
  LabelBuilder& left(float v) {
    get()->left(v);
    return *this;
  }
  LabelBuilder& top(float v) {
    get()->top(v);
    return *this;
  }
  LabelBuilder& right(float v) {
    get()->right(v);
    return *this;
  }
  LabelBuilder& bottom(float v) {
    get()->bottom(v);
    return *this;
  }
  LabelBuilder& preferred_height(float h) {
    get()->preferred_height(h);
    return *this;
  }
};

class ButtonBuilder : public detail::BuilderBase<Button> {
 public:
  ButtonBuilder& text(const std::wstring& t) {
    get()->text(t);
    return *this;
  }
  ButtonBuilder& font_size(float size) {
    get()->font_size(size);
    return *this;
  }
  ButtonBuilder& preferred_size(float w, float h) {
    get()->preferred_size(w, h);
    return *this;
  }
  ButtonBuilder& pos(float x, float y) {
    get()->set_pos(x, y);
    return *this;
  }
  ButtonBuilder& left(float v) {
    get()->left(v);
    return *this;
  }
  ButtonBuilder& top(float v) {
    get()->top(v);
    return *this;
  }
  ButtonBuilder& right(float v) {
    get()->right(v);
    return *this;
  }
  ButtonBuilder& bottom(float v) {
    get()->bottom(v);
    return *this;
  }
  ButtonBuilder& weight(float w) {
    get()->weight(w);
    return *this;
  }
  ButtonBuilder& cross_align(Align a) {
    get()->cross_align(a);
    return *this;
  }
  ButtonBuilder& fill_height() {
    get()->fill_height();
    return *this;
  }
  ButtonBuilder& fixed_height(float h) {
    get()->fixed_height(h);
    return *this;
  }
  ButtonBuilder& hug_width() {
    get()->hug_width();
    return *this;
  }
  ButtonBuilder& fixed_width(float w) {
    get()->fixed_width(w);
    return *this;
  }
  ButtonBuilder& on_click(Button::ClickHandler handler) {
    get()->on_click(std::move(handler));
    return *this;
  }
  ButtonBuilder& bg(const ColorF& c) {
    get()->bg(c);
    return *this;
  }
  ButtonBuilder& bg_hover(const ColorF& c) {
    get()->bg_hover(c);
    return *this;
  }
  ButtonBuilder& bg_pressed(const ColorF& c) {
    get()->bg_pressed(c);
    return *this;
  }
  ButtonBuilder& text_color(const ColorF& c) {
    get()->text_color(c);
    return *this;
  }
  ButtonBuilder& text_align(auralite::TextHAlign a) {
    get()->text_align(a);
    return *this;
  }
  ButtonBuilder& corner_radius(float r) {
    get()->corner_radius(r);
    return *this;
  }
};

class TextFieldBuilder : public detail::BuilderBase<TextField> {
 public:
  TextFieldBuilder& text(const std::wstring& t) {
    get()->text(t);
    return *this;
  }
  TextFieldBuilder& placeholder(const std::wstring& t) {
    get()->placeholder(t);
    return *this;
  }
  TextFieldBuilder& password(bool enabled) {
    get()->password(enabled);
    return *this;
  }
  TextFieldBuilder& font_size(float size) {
    get()->font_size(size);
    return *this;
  }
  TextFieldBuilder& preferred_size(float w, float h) {
    get()->preferred_size(w, h);
    return *this;
  }
  TextFieldBuilder& on_change(TextField::ChangeHandler handler) {
    get()->on_change(std::move(handler));
    return *this;
  }
};

class ImageViewBuilder : public detail::BuilderBase<ImageView> {
 public:
  ImageViewBuilder& path(const std::wstring& p) {
    get()->LoadFromFile(p);
    return *this;
  }
  ImageViewBuilder& preferred_size(float w, float h) {
    get()->preferred_size(w, h);
    return *this;
  }
};

class ImageButtonBuilder : public detail::BuilderBase<ImageButton> {
 public:
  ImageButtonBuilder& preferred_size(float w, float h) {
    get()->preferred_size(w, h);
    return *this;
  }
  ImageButtonBuilder& on_click(ImageButton::ClickHandler handler) {
    get()->on_click(std::move(handler));
    return *this;
  }
};

class CheckboxBuilder : public detail::BuilderBase<Checkbox> {
 public:
  CheckboxBuilder& text(const std::wstring& t) {
    get()->text(t);
    return *this;
  }
  CheckboxBuilder& font_size(float size) {
    get()->font_size(size);
    return *this;
  }
  CheckboxBuilder& checked(bool v) {
    get()->checked(v);
    return *this;
  }
  CheckboxBuilder& on_changed(Checkbox::ChangedHandler handler) {
    get()->on_changed(std::move(handler));
    return *this;
  }
};

class RadioBuilder : public detail::BuilderBase<Radio> {
 public:
  RadioBuilder& text(const std::wstring& t) {
    get()->text(t);
    return *this;
  }
  RadioBuilder& font_size(float size) {
    get()->font_size(size);
    return *this;
  }
  RadioBuilder& group_id(int id) {
    get()->group_id(id);
    return *this;
  }
  RadioBuilder& checked(bool v) {
    get()->checked(v);
    return *this;
  }
  RadioBuilder& on_changed(Radio::ChangedHandler handler) {
    get()->on_changed(std::move(handler));
    return *this;
  }
};

class SwitchBuilder : public detail::BuilderBase<Switch> {
 public:
  SwitchBuilder& text(const std::wstring& t) {
    get()->text(t);
    return *this;
  }
  SwitchBuilder& font_size(float size) {
    get()->font_size(size);
    return *this;
  }
  SwitchBuilder& on(bool v) {
    get()->on(v);
    return *this;
  }
  SwitchBuilder& on_changed(Switch::ChangedHandler handler) {
    get()->on_changed(std::move(handler));
    return *this;
  }
};

class ScrollViewBuilder : public detail::BuilderBase<ScrollView> {
 public:
  ScrollViewBuilder& preferred_size(float w, float h) {
    get()->preferred_size(w, h);
    return *this;
  }
  ScrollViewBuilder& fill_width() {
    get()->fill_width();
    return *this;
  }
  ScrollViewBuilder& fill_height() {
    get()->fill_height();
    return *this;
  }
  ScrollViewBuilder& fixed_height(float h) {
    get()->fixed_height(h);
    return *this;
  }
  ScrollViewBuilder& content(std::unique_ptr<Node> n) {
    get()->set_content(std::move(n));
    return *this;
  }
  template <typename B>
  ScrollViewBuilder& content(B&& builder) {
    return content(std::forward<B>(builder).Build());
  }
};

class SubmenuBuilder : public detail::BuilderBase<Submenu> {
 public:
  SubmenuBuilder& text(const std::wstring& t) {
    get()->text(t);
    return *this;
  }
  SubmenuBuilder& content(std::unique_ptr<Node> n) {
    get()->content(std::move(n));
    return *this;
  }
  template <typename B>
  SubmenuBuilder& content(B&& builder) {
    return content(std::forward<B>(builder).Build());
  }
  SubmenuBuilder& open_on_hover(bool v) {
    get()->open_on_hover(v);
    return *this;
  }
};

class ListViewBuilder : public detail::BuilderBase<ListView> {
 public:
  ListViewBuilder& font_size(float size) {
    get()->font_size(size);
    return *this;
  }
  ListViewBuilder& item(const std::wstring& text) {
    get()->AddItem(text);
    return *this;
  }
  ListViewBuilder& on_selection_changed(ListView::SelectionHandler handler) {
    get()->on_selection_changed(std::move(handler));
    return *this;
  }
};

class SplitViewBuilder : public detail::BuilderBase<SplitView> {
 public:
  SplitViewBuilder& preferred_size(float w, float h) {
    get()->preferred_size(w, h);
    return *this;
  }
  SplitViewBuilder& fill_width() {
    get()->fill_width();
    return *this;
  }
  SplitViewBuilder& fixed_height(float h) {
    get()->fixed_height(h);
    return *this;
  }
  SplitViewBuilder& ratio(float r) {
    get()->set_ratio(r);
    return *this;
  }
  SplitViewBuilder& leading(std::unique_ptr<Node> n) {
    get()->set_leading(std::move(n));
    return *this;
  }
  SplitViewBuilder& trailing(std::unique_ptr<Node> n) {
    get()->set_trailing(std::move(n));
    return *this;
  }
  template <typename B>
  SplitViewBuilder& leading(B&& builder) {
    return leading(std::forward<B>(builder).Build());
  }
  template <typename B>
  SplitViewBuilder& trailing(B&& builder) {
    return trailing(std::forward<B>(builder).Build());
  }
};

inline ColumnBuilder Column() { return ColumnBuilder(); }
inline RowBuilder Row() { return RowBuilder(); }
inline TileBuilder Tile() { return TileBuilder(); }
inline TabBuilder Tab() { return TabBuilder(); }
inline AbsoluteBuilder Absolute() { return AbsoluteBuilder(); }
inline LabelBuilder Label() { return LabelBuilder(); }
inline ButtonBuilder Button() { return ButtonBuilder(); }

class UserControlBuilder : public detail::BuilderBase<UserControl> {
 public:
  UserControlBuilder& on_paint(UserControl::PaintHandler handler) {
    get()->on_paint(std::move(handler));
    return *this;
  }
  UserControlBuilder& on_mouse_down(UserControl::MouseHandler handler) {
    get()->on_mouse_down(std::move(handler));
    return *this;
  }
  UserControlBuilder& on_mouse_up(UserControl::MouseHandler handler) {
    get()->on_mouse_up(std::move(handler));
    return *this;
  }
  UserControlBuilder& on_mouse_move(UserControl::MouseHandler handler) {
    get()->on_mouse_move(std::move(handler));
    return *this;
  }
  UserControlBuilder& on_mouse_wheel(UserControl::MouseHandler handler) {
    get()->on_mouse_wheel(std::move(handler));
    return *this;
  }
  UserControlBuilder& on_key(UserControl::KeyHandler handler) {
    get()->on_key(std::move(handler));
    return *this;
  }
  UserControlBuilder& wants_mouse_wheel(bool want) {
    get()->wants_mouse_wheel(want);
    return *this;
  }
  UserControlBuilder& fill_width() {
    get()->fill_width();
    return *this;
  }
  UserControlBuilder& fill_height() {
    get()->fill_height();
    return *this;
  }
  UserControlBuilder& weight(float w) {
    get()->weight(w);
    return *this;
  }
};

inline UserControlBuilder UserControl() { return UserControlBuilder(); }
inline TextFieldBuilder TextField() { return TextFieldBuilder(); }
inline ImageViewBuilder ImageView() { return ImageViewBuilder(); }
inline ImageButtonBuilder ImageButton() { return ImageButtonBuilder(); }
inline CheckboxBuilder Checkbox() { return CheckboxBuilder(); }
inline RadioBuilder Radio() { return RadioBuilder(); }
inline SwitchBuilder Switch() { return SwitchBuilder(); }
inline ScrollViewBuilder ScrollView() { return ScrollViewBuilder(); }
inline SubmenuBuilder Submenu() { return SubmenuBuilder(); }
inline ListViewBuilder ListView() { return ListViewBuilder(); }
inline SplitViewBuilder SplitView() { return SplitViewBuilder(); }

class ProgressBarBuilder : public detail::BuilderBase<ProgressBar> {
 public:
  ProgressBarBuilder& value(float v) {
    get()->value(v);
    return *this;
  }
  ProgressBarBuilder& indeterminate(bool enable) {
    get()->indeterminate(enable);
    return *this;
  }
  ProgressBarBuilder& fill_width() {
    get()->fill_width();
    return *this;
  }
  ProgressBarBuilder& fixed_height(float h) {
    get()->fixed_height(h);
    return *this;
  }
};

class SliderBuilder : public detail::BuilderBase<Slider> {
 public:
  SliderBuilder& value(float v) {
    get()->value(v);
    return *this;
  }
  SliderBuilder& step(float s) {
    get()->step(s);
    return *this;
  }
  SliderBuilder& tick_count(int n) {
    get()->tick_count(n);
    return *this;
  }
  SliderBuilder& orientation(SliderOrientation o) {
    get()->orientation(o);
    return *this;
  }
  SliderBuilder& on_changed(Slider::ChangeHandler handler) {
    get()->on_changed(std::move(handler));
    return *this;
  }
  SliderBuilder& fill_width() {
    get()->fill_width();
    return *this;
  }
  SliderBuilder& fill_height() {
    get()->fill_height();
    return *this;
  }
  SliderBuilder& fixed_width(float w) {
    get()->fixed_width(w);
    return *this;
  }
  SliderBuilder& fixed_height(float h) {
    get()->fixed_height(h);
    return *this;
  }
};

class ComboBuilder : public detail::BuilderBase<Combo> {
 public:
  ComboBuilder& items(std::vector<std::wstring> values) {
    get()->items(std::move(values));
    return *this;
  }
  ComboBuilder& selected(int index) {
    get()->selected(index);
    return *this;
  }
  ComboBuilder& selected_indices(std::vector<int> indices) {
    get()->selected_indices(std::move(indices));
    return *this;
  }
  ComboBuilder& editable(bool enable) {
    get()->editable(enable);
    return *this;
  }
  ComboBuilder& multi(bool enable) {
    get()->multi(enable);
    return *this;
  }
  ComboBuilder& on_changed(Combo::ChangeHandler handler) {
    get()->on_changed(std::move(handler));
    return *this;
  }
  ComboBuilder& on_multi_changed(Combo::MultiChangeHandler handler) {
    get()->on_multi_changed(std::move(handler));
    return *this;
  }
  ComboBuilder& fill_width() {
    get()->fill_width();
    return *this;
  }
};

class TextAreaBuilder : public detail::BuilderBase<TextArea> {
 public:
  TextAreaBuilder& text(const std::wstring& t) {
    get()->text(t);
    return *this;
  }
  TextAreaBuilder& placeholder(const std::wstring& t) {
    get()->placeholder(t);
    return *this;
  }
  TextAreaBuilder& font_size(float size) {
    get()->font_size(size);
    return *this;
  }
  TextAreaBuilder& wrap(bool enable) {
    get()->wrap(enable);
    return *this;
  }
  TextAreaBuilder& on_change(TextArea::ChangeHandler handler) {
    get()->on_change(std::move(handler));
    return *this;
  }
  TextAreaBuilder& fill_width() {
    get()->fill_width();
    return *this;
  }
  TextAreaBuilder& fixed_height(float h) {
    get()->fixed_height(h);
    return *this;
  }
};

class VirtualListBuilder : public detail::BuilderBase<VirtualList> {
 public:
  VirtualListBuilder& item_count(VirtualList::ItemCountFn fn) {
    get()->item_count(std::move(fn));
    return *this;
  }
  VirtualListBuilder& item_kind(VirtualList::ItemKindFn fn) {
    get()->item_kind(std::move(fn));
    return *this;
  }
  VirtualListBuilder& item_text(VirtualList::ItemTextFn fn) {
    get()->item_text(std::move(fn));
    return *this;
  }
  VirtualListBuilder& item_sub_text(VirtualList::ItemSubTextFn fn) {
    get()->item_sub_text(std::move(fn));
    return *this;
  }
  VirtualListBuilder& item_cell_text(VirtualList::ItemCellTextFn fn) {
    get()->item_cell_text(std::move(fn));
    return *this;
  }
  VirtualListBuilder& item_checked(VirtualList::ItemCheckedFn fn) {
    get()->item_checked(std::move(fn));
    return *this;
  }
  VirtualListBuilder& item_set_checked(VirtualList::ItemCheckSetFn fn) {
    get()->item_set_checked(std::move(fn));
    return *this;
  }
  VirtualListBuilder& on_paint_item(VirtualList::PaintItemFn fn) {
    get()->on_paint_item(std::move(fn));
    return *this;
  }
  VirtualListBuilder& on_selection_changed(VirtualList::SelectionHandler h) {
    get()->on_selection_changed(std::move(h));
    return *this;
  }
  VirtualListBuilder& on_check_changed(VirtualList::CheckHandler h) {
    get()->on_check_changed(std::move(h));
    return *this;
  }
  VirtualListBuilder& font_size(float size) {
    get()->font_size(size);
    return *this;
  }
  VirtualListBuilder& overscan(int rows) {
    get()->overscan(rows);
    return *this;
  }
  VirtualListBuilder& columns(std::vector<ListColumn> cols) {
    get()->columns(std::move(cols));
    return *this;
  }
  VirtualListBuilder& show_header(bool v) {
    get()->show_header(v);
    return *this;
  }
  VirtualListBuilder& header_height(float h) {
    get()->header_height(h);
    return *this;
  }
  VirtualListBuilder& frozen_count(int n) {
    get()->frozen_count(n);
    return *this;
  }
  VirtualListBuilder& on_sort_changed(VirtualList::SortHandler h) {
    get()->on_sort_changed(std::move(h));
    return *this;
  }
  VirtualListBuilder& fill_width() {
    get()->fill_width();
    return *this;
  }
  VirtualListBuilder& fixed_height(float h) {
    get()->fixed_height(h);
    return *this;
  }
  VirtualListBuilder& fill_height() {
    get()->fill_height();
    return *this;
  }
};

class TreeViewBuilder : public detail::BuilderBase<TreeView> {
 public:
  TreeViewBuilder& font_size(float size) {
    get()->font_size(size);
    return *this;
  }
  TreeViewBuilder& row_height(float h) {
    get()->row_height(h);
    return *this;
  }
  TreeViewBuilder& indent(float px) {
    get()->indent(px);
    return *this;
  }
  TreeViewBuilder& on_selection_changed(TreeView::SelectionHandler h) {
    get()->on_selection_changed(std::move(h));
    return *this;
  }
  TreeViewBuilder& on_expanded_changed(TreeView::ExpandHandler h) {
    get()->on_expanded_changed(std::move(h));
    return *this;
  }
  TreeViewBuilder& checkable(bool v) {
    get()->checkable(v);
    return *this;
  }
  TreeViewBuilder& check_cascade(bool v) {
    get()->check_cascade(v);
    return *this;
  }
  TreeViewBuilder& on_check_changed(TreeView::CheckHandler h) {
    get()->on_check_changed(std::move(h));
    return *this;
  }
  TreeViewBuilder& on_load_children(TreeView::LoadChildrenHandler h) {
    get()->on_load_children(std::move(h));
    return *this;
  }
  TreeViewBuilder& fill_width() {
    get()->fill_width();
    return *this;
  }
  TreeViewBuilder& fixed_height(float h) {
    get()->fixed_height(h);
    return *this;
  }
};

class ItemListBuilder : public detail::BuilderBase<ItemList> {
 public:
  ItemListBuilder& add_item(ItemList::PaintItemFn paint = {}) {
    get()->AddItem(std::move(paint));
    return *this;
  }
  ItemListBuilder& item_count(int n) {
    get()->set_item_count(n);
    return *this;
  }
  ItemListBuilder& on_paint_item(ItemList::PaintItemFn fn) {
    get()->on_paint_item(std::move(fn));
    return *this;
  }
  ItemListBuilder& on_bind_item(ItemList::BindItemFn fn) {
    get()->on_bind_item(std::move(fn));
    return *this;
  }
  ItemListBuilder& item_template_factory(ItemList::ItemTemplateFactory fn) {
    get()->item_template_factory(std::move(fn));
    return *this;
  }
  ItemListBuilder& on_selection_changed(ItemList::SelectionHandler h) {
    get()->on_selection_changed(std::move(h));
    return *this;
  }
  ItemListBuilder& row_height(float h) {
    get()->row_height(h);
    return *this;
  }
  ItemListBuilder& overscan(int rows) {
    get()->overscan(rows);
    return *this;
  }
  ItemListBuilder& row_padding(float pad) {
    get()->row_padding(pad);
    return *this;
  }
  ItemListBuilder& header_height(float h) {
    get()->header_height(h);
    return *this;
  }
  ItemListBuilder& columns(std::vector<ListColumn> cols) {
    get()->columns(std::move(cols));
    return *this;
  }
  ItemListBuilder& show_header(bool v) {
    get()->show_header(v);
    return *this;
  }
  ItemListBuilder& frozen_count(int n) {
    get()->frozen_count(n);
    return *this;
  }
  ItemListBuilder& on_sort_changed(ItemList::SortHandler h) {
    get()->on_sort_changed(std::move(h));
    return *this;
  }
  ItemListBuilder& selected(int index) {
    get()->set_selected_index(index, false);
    return *this;
  }
  ItemListBuilder& fill_width() {
    get()->fill_width();
    return *this;
  }
  ItemListBuilder& fill_height() {
    get()->fill_height();
    return *this;
  }
  ItemListBuilder& fixed_height(float h) {
    get()->fixed_height(h);
    return *this;
  }
};

inline ProgressBarBuilder ProgressBar() { return ProgressBarBuilder(); }
inline SliderBuilder Slider() { return SliderBuilder(); }
inline ComboBuilder Combo() { return ComboBuilder(); }
inline TextAreaBuilder TextArea() { return TextAreaBuilder(); }
inline VirtualListBuilder VirtualList() { return VirtualListBuilder(); }
inline TreeViewBuilder TreeView() { return TreeViewBuilder(); }
inline ItemListBuilder ItemList() { return ItemListBuilder(); }

}  // namespace auralite::ui::dsl
