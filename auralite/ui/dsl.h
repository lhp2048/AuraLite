#pragma once

#include "auralite/ui/button.h"
#include "auralite/ui/checkbox.h"
#include "auralite/ui/column.h"
#include "auralite/ui/image_button.h"
#include "auralite/ui/image_view.h"
#include "auralite/ui/label.h"
#include "auralite/ui/list_view.h"
#include "auralite/ui/radio.h"
#include "auralite/ui/row.h"
#include "auralite/ui/scroll_view.h"
#include "auralite/ui/split_view.h"
#include "auralite/ui/switch_control.h"
#include "auralite/ui/text_field.h"

#include <memory>
#include <string>
#include <utility>

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

}  // namespace auralite::ui::dsl
