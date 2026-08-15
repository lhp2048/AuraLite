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
#include "auralite/ui/progress_bar.h"
#include "auralite/ui/radio.h"
#include "auralite/ui/row.h"
#include "auralite/ui/scroll_view.h"
#include "auralite/ui/slider.h"
#include "auralite/ui/split_view.h"
#include "auralite/ui/switch_control.h"
#include "auralite/ui/tab.h"
#include "auralite/ui/text_area.h"
#include "auralite/ui/text_field.h"
#include "auralite/ui/tile.h"

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
inline TextFieldBuilder TextField() { return TextFieldBuilder(); }
inline ImageViewBuilder ImageView() { return ImageViewBuilder(); }
inline ImageButtonBuilder ImageButton() { return ImageButtonBuilder(); }
inline CheckboxBuilder Checkbox() { return CheckboxBuilder(); }
inline RadioBuilder Radio() { return RadioBuilder(); }
inline SwitchBuilder Switch() { return SwitchBuilder(); }
inline ScrollViewBuilder ScrollView() { return ScrollViewBuilder(); }
inline ListViewBuilder ListView() { return ListViewBuilder(); }
inline SplitViewBuilder SplitView() { return SplitViewBuilder(); }

class ProgressBarBuilder : public detail::BuilderBase<ProgressBar> {
 public:
  ProgressBarBuilder& value(float v) {
    get()->value(v);
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
  SliderBuilder& on_changed(Slider::ChangeHandler handler) {
    get()->on_changed(std::move(handler));
    return *this;
  }
  SliderBuilder& fill_width() {
    get()->fill_width();
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
  ComboBuilder& on_changed(Combo::ChangeHandler handler) {
    get()->on_changed(std::move(handler));
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

inline ProgressBarBuilder ProgressBar() { return ProgressBarBuilder(); }
inline SliderBuilder Slider() { return SliderBuilder(); }
inline ComboBuilder Combo() { return ComboBuilder(); }
inline TextAreaBuilder TextArea() { return TextAreaBuilder(); }

}  // namespace auralite::ui::dsl
