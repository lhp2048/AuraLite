#include "auralite/ui/factory.h"

#include "auralite/ui/absolute.h"
#include "auralite/ui/button.h"
#include "auralite/ui/checkbox.h"
#include "auralite/ui/column.h"
#include "auralite/ui/combo.h"
#include "auralite/ui/image_button.h"
#include "auralite/ui/image_view.h"
#include "auralite/ui/label.h"
#include "auralite/ui/list_view.h"
#include "auralite/ui/list_columns.h"
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
#include "auralite/ui/yaml_loader.h"
#include "auralite/ui/theme_yaml.h"

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <utility>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace auralite::ui {
namespace {

std::string WideToUtf8(const std::wstring& wide) {
  if (wide.empty()) {
    return {};
  }
  const int n = WideCharToMultiByte(CP_UTF8, 0, wide.data(),
                                    static_cast<int>(wide.size()), nullptr, 0,
                                    nullptr, nullptr);
  if (n <= 0) {
    return {};
  }
  std::string out(static_cast<size_t>(n), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                      out.data(), n, nullptr, nullptr);
  return out;
}

TextAlign ParseTextAlign(const std::string& s) {
  if (s == "center" || s == "Center") {
    return TextAlign::Center;
  }
  if (s == "right" || s == "Right") {
    return TextAlign::Right;
  }
  return TextAlign::Left;
}

Align ParseCrossAlign(const std::string& s) {
  if (s == "center" || s == "Center") {
    return Align::Center;
  }
  if (s == "end" || s == "End" || s == "right" || s == "Right") {
    return Align::End;
  }
  return Align::Start;
}

void ApplyPaddingColumn(Column* col, const YAML::Node& props) {
  if (!col || !props["padding"]) {
    return;
  }
  const YAML::Node& p = props["padding"];
  if (p.IsSequence() && p.size() >= 4) {
    col->padding(p[0].as<float>(), p[1].as<float>(), p[2].as<float>(),
                 p[3].as<float>());
  } else {
    col->padding(p.as<float>());
  }
}

void ApplyPaddingRow(Row* row, const YAML::Node& props) {
  if (!row || !props["padding"]) {
    return;
  }
  const YAML::Node& p = props["padding"];
  if (p.IsSequence() && p.size() >= 4) {
    row->padding(p[0].as<float>(), p[1].as<float>(), p[2].as<float>(),
                 p[3].as<float>());
  } else {
    row->padding(p.as<float>());
  }
}

void BindOnClick(Button* btn, const YAML::Node& props,
                 const HandlerMap& handlers) {
  if (!btn || !props["on_click"]) {
    return;
  }
  const std::string name = props["on_click"].as<std::string>();
  const auto it = handlers.find(name);
  if (it != handlers.end()) {
    btn->on_click(it->second);
  }
}

template <typename T>
void ApplyOptionalColor(T* ctrl, const YAML::Node& props, const char* key,
                        T& (T::*setter)(const ColorF&)) {
  if (!ctrl || !props[key]) {
    return;
  }
  ColorF c;
  if (ParseColorHex(props[key].as<std::string>(), &c)) {
    (ctrl->*setter)(c);
  }
}

void ApplyButtonChrome(Button* btn, const YAML::Node& props) {
  if (!btn) {
    return;
  }
  ApplyOptionalColor(btn, props, "bg", &Button::bg);
  ApplyOptionalColor(btn, props, "bg_hover", &Button::bg_hover);
  ApplyOptionalColor(btn, props, "bg_pressed", &Button::bg_pressed);
  ApplyOptionalColor(btn, props, "text_color", &Button::text_color);
  if (props["corner_radius"]) {
    btn->corner_radius(props["corner_radius"].as<float>());
  }
  if (props["text_align"]) {
    const std::string a = props["text_align"].as<std::string>();
    if (a == "left" || a == "Left") {
      btn->text_align(auralite::TextHAlign::Left);
    } else if (a == "right" || a == "Right") {
      btn->text_align(auralite::TextHAlign::Right);
    } else {
      btn->text_align(auralite::TextHAlign::Center);
    }
  }
}

void ApplySubmenuChrome(Submenu* sm, const YAML::Node& props) {
  if (!sm) {
    return;
  }
  ApplyOptionalColor(sm, props, "bg", &Submenu::bg);
  ApplyOptionalColor(sm, props, "bg_hover", &Submenu::bg_hover);
  ApplyOptionalColor(sm, props, "text_color", &Submenu::text_color);
  if (props["corner_radius"]) {
    sm->corner_radius(props["corner_radius"].as<float>());
  }
  if (props["font_size"]) {
    sm->font_size(props["font_size"].as<float>());
  }
}

// width/height: number → Fixed; "fill"/"hug" → policy. Omitting keeps control defaults.
void ApplySizeAxis(Node* node, const YAML::Node& props, const char* key,
                   bool is_width) {
  if (!node || !props[key]) {
    return;
  }
  const YAML::Node& n = props[key];
  if (!n.IsScalar()) {
    return;
  }
  try {
    const float v = n.as<float>();
    if (is_width) {
      node->fixed_width(v);
    } else {
      node->fixed_height(v);
    }
    return;
  } catch (const YAML::Exception&) {
  }
  const std::string s = n.as<std::string>();
  if (s == "fill" || s == "Fill") {
    if (is_width) {
      node->fill_width();
    } else {
      node->fill_height();
    }
  } else if (s == "hug" || s == "Hug") {
    if (is_width) {
      node->hug_width();
    } else {
      node->hug_height();
    }
  }
}

void ApplyWidthHeight(Node* node, const YAML::Node& props) {
  ApplySizeAxis(node, props, "width", true);
  ApplySizeAxis(node, props, "height", false);
}

std::vector<ListColumn> ParseListColumns(const YAML::Node& props) {
  std::vector<ListColumn> cols;
  if (!props["columns"] || !props["columns"].IsSequence()) {
    return cols;
  }
  for (const auto& c : props["columns"]) {
    ListColumn col;
    if (c.IsScalar()) {
      col.title = Utf8ToWide(c.as<std::string>());
    } else if (c.IsMap()) {
      if (c["title"]) {
        col.title = Utf8ToWide(c["title"].as<std::string>());
      }
      if (c["width"]) {
        col.width = c["width"].as<float>();
      }
      if (c["weight"]) {
        col.weight = c["weight"].as<float>();
      }
      if (c["align"]) {
        const std::string a = c["align"].as<std::string>();
        if (a == "center") {
          col.align = TextAlign::Center;
        } else if (a == "right") {
          col.align = TextAlign::Right;
        }
      }
      if (c["sortable"]) {
        col.sortable = c["sortable"].as<bool>();
      }
      if (c["resizable"]) {
        col.resizable = c["resizable"].as<bool>();
      }
    }
    cols.push_back(std::move(col));
  }
  return cols;
}

void ApplyListColumnsHeader(auto* list, const YAML::Node& props) {
  auto cols = ParseListColumns(props);
  if (!cols.empty()) {
    list->columns(std::move(cols));
  }
  if (props["show_header"]) {
    list->show_header(props["show_header"].as<bool>());
  }
  if (props["header_height"]) {
    list->header_height(props["header_height"].as<float>());
  }
  if (props["frozen_count"]) {
    list->frozen_count(props["frozen_count"].as<int>());
  }
}

void ApplyWeightCrossAlign(Node* node, const YAML::Node& props,
                           bool label_text_align) {
  if (!node) {
    return;
  }
  if (props["weight"]) {
    node->weight(props["weight"].as<float>());
  }
  if (props["cross_align"]) {
    node->cross_align(ParseCrossAlign(props["cross_align"].as<std::string>()));
  } else if (!label_text_align && props["align"]) {
    // Non-Label: align is layout cross_align alias (DuiLib-ish).
    node->cross_align(ParseCrossAlign(props["align"].as<std::string>()));
  }
}

void ApplyPos(Node* node, const YAML::Node& props) {
  if (!node) {
    return;
  }
  if (props["x"] || props["y"]) {
    const float x = props["x"] ? props["x"].as<float>() : 0.f;
    const float y = props["y"] ? props["y"].as<float>() : 0.f;
    node->set_pos(x, y);
  }
}

void ApplyAnchors(Node* node, const YAML::Node& props) {
  if (!node) {
    return;
  }
  if (props["left"]) {
    node->left(props["left"].as<float>());
  }
  if (props["top"]) {
    node->top(props["top"].as<float>());
  }
  if (props["right"]) {
    node->right(props["right"].as<float>());
  }
  if (props["bottom"]) {
    node->bottom(props["bottom"].as<float>());
  }
}

void BindOnClick(ImageButton* btn, const YAML::Node& props,
                 const HandlerMap& handlers) {
  if (!btn || !props["on_click"]) {
    return;
  }
  const std::string name = props["on_click"].as<std::string>();
  const auto it = handlers.find(name);
  if (it != handlers.end()) {
    btn->on_click(it->second);
  }
}

std::string NodeTypeName(const Node* n) {
  if (!n) {
    return "null";
  }
  if (dynamic_cast<const Column*>(n)) {
    return "Column";
  }
  if (dynamic_cast<const Row*>(n)) {
    return "Row";
  }
  if (dynamic_cast<const Tile*>(n)) {
    return "Tile";
  }
  if (dynamic_cast<const Tab*>(n)) {
    return "Tab";
  }
  if (dynamic_cast<const Absolute*>(n)) {
    return "Absolute";
  }
  if (dynamic_cast<const Label*>(n)) {
    return "Label";
  }
  if (dynamic_cast<const Button*>(n)) {
    return "Button";
  }
  if (dynamic_cast<const TextField*>(n)) {
    return "TextField";
  }
  if (dynamic_cast<const ImageView*>(n)) {
    return "ImageView";
  }
  if (dynamic_cast<const ImageButton*>(n)) {
    return "ImageButton";
  }
  if (dynamic_cast<const Checkbox*>(n)) {
    return "Checkbox";
  }
  if (dynamic_cast<const Radio*>(n)) {
    return "Radio";
  }
  if (dynamic_cast<const Switch*>(n)) {
    return "Switch";
  }
  if (dynamic_cast<const ScrollView*>(n)) {
    return "ScrollView";
  }
  if (dynamic_cast<const Submenu*>(n)) {
    return "Submenu";
  }
  if (dynamic_cast<const ListView*>(n)) {
    return "ListView";
  }
  if (dynamic_cast<const ItemList*>(n)) {
    return "ItemList";
  }
  if (dynamic_cast<const VirtualList*>(n)) {
    return "VirtualList";
  }
  if (dynamic_cast<const TreeView*>(n)) {
    return "TreeView";
  }
  if (dynamic_cast<const ProgressBar*>(n)) {
    return "ProgressBar";
  }
  if (dynamic_cast<const Slider*>(n)) {
    return "Slider";
  }
  if (dynamic_cast<const Combo*>(n)) {
    return "Combo";
  }
  if (dynamic_cast<const TextArea*>(n)) {
    return "TextArea";
  }
  if (dynamic_cast<const SplitView*>(n)) {
    return "SplitView";
  }
  return "Node";
}

std::string NodeDetail(const Node* n) {
  if (const auto* label = dynamic_cast<const Label*>(n)) {
    return " text=\"" + WideToUtf8(label->text()) + "\"";
  }
  if (const auto* btn = dynamic_cast<const Button*>(n)) {
    return " text=\"" + WideToUtf8(btn->text()) + "\"";
  }
  if (const auto* cb = dynamic_cast<const Checkbox*>(n)) {
    return " text=\"" + WideToUtf8(cb->text()) + "\"";
  }
  if (const auto* radio = dynamic_cast<const Radio*>(n)) {
    return " text=\"" + WideToUtf8(radio->text()) + "\"";
  }
  if (const auto* sw = dynamic_cast<const Switch*>(n)) {
    return " text=\"" + WideToUtf8(sw->text()) + "\"";
  }
  if (const auto* col = dynamic_cast<const Column*>(n)) {
    return " spacing=" + std::to_string(static_cast<int>(col->spacing()));
  }
  if (const auto* row = dynamic_cast<const Row*>(n)) {
    return " spacing=" + std::to_string(static_cast<int>(row->spacing()));
  }
  if (const auto* tile = dynamic_cast<const Tile*>(n)) {
    return " columns=" + std::to_string(tile->columns());
  }
  if (const auto* tab = dynamic_cast<const Tab*>(n)) {
    return " selected=" + std::to_string(tab->selected());
  }
  if (const auto* list = dynamic_cast<const ListView*>(n)) {
    return " items=" + std::to_string(list->item_count());
  }
  if (const auto* split = dynamic_cast<const SplitView*>(n)) {
    return " ratio=" + std::to_string(split->ratio());
  }
  return {};
}

void DumpTreeRec(const Node* n, int depth, std::ostringstream& out) {
  if (!n) {
    return;
  }
  for (int i = 0; i < depth; ++i) {
    out << "  ";
  }
  out << NodeTypeName(n) << NodeDetail(n) << '\n';
  for (const auto& child : n->children()) {
    DumpTreeRec(child.get(), depth + 1, out);
  }
}

}  // namespace

ViewFactory::ViewFactory() { RegisterBuiltinTypes(); }

void ViewFactory::Register(const std::string& type, NodeBuilder builder) {
  builders_[type] = std::move(builder);
}

bool ViewFactory::HasType(const std::string& type) const {
  return builders_.find(type) != builders_.end();
}

std::unique_ptr<Node> ViewFactory::Build(const std::string& type,
                                         const YAML::Node& props,
                                         const HandlerMap& handlers) const {
  const auto it = builders_.find(type);
  if (it == builders_.end() || !it->second) {
    return nullptr;
  }
  return it->second(props, handlers);
}

void ViewFactory::RegisterBuiltinTypes() {
  Register("Column", [](const YAML::Node& props, const HandlerMap&) {
    auto col = std::make_unique<Column>();
    ApplyPaddingColumn(col.get(), props);
    if (props["spacing"]) {
      col->spacing(props["spacing"].as<float>());
    }
    if (props["child_align"]) {
      col->child_align(ParseCrossAlign(props["child_align"].as<std::string>()));
    }
    if (props["main_align"]) {
      col->main_align(ParseCrossAlign(props["main_align"].as<std::string>()));
    }
    ApplyWidthHeight(col.get(), props);
    ApplyWeightCrossAlign(col.get(), props, false);
    return col;
  });

  Register("Row", [](const YAML::Node& props, const HandlerMap&) {
    auto row = std::make_unique<Row>();
    ApplyPaddingRow(row.get(), props);
    if (props["spacing"]) {
      row->spacing(props["spacing"].as<float>());
    }
    if (props["child_align"]) {
      row->child_align(ParseCrossAlign(props["child_align"].as<std::string>()));
    }
    if (props["main_align"]) {
      row->main_align(ParseCrossAlign(props["main_align"].as<std::string>()));
    }
    ApplyWidthHeight(row.get(), props);
    ApplyWeightCrossAlign(row.get(), props, false);
    return row;
  });

  Register("Tile", [](const YAML::Node& props, const HandlerMap&) {
    auto tile = std::make_unique<Tile>();
    if (props["padding"]) {
      const YAML::Node& p = props["padding"];
      if (p.IsSequence() && p.size() >= 4) {
        tile->padding(p[0].as<float>(), p[1].as<float>(), p[2].as<float>(),
                      p[3].as<float>());
      } else {
        tile->padding(p.as<float>());
      }
    }
    if (props["spacing"]) {
      const YAML::Node& s = props["spacing"];
      if (s.IsSequence() && s.size() >= 2) {
        tile->spacing(s[0].as<float>(), s[1].as<float>());
      } else {
        tile->spacing(s.as<float>());
      }
    }
    if (props["columns"]) {
      tile->columns(props["columns"].as<int>());
    }
    if (props["item_width"] || props["item_height"] || props["item_size"]) {
      float iw = 80.f;
      float ih = 80.f;
      if (props["item_size"] && props["item_size"].IsSequence() &&
          props["item_size"].size() >= 2) {
        iw = props["item_size"][0].as<float>();
        ih = props["item_size"][1].as<float>();
      }
      if (props["item_width"]) {
        iw = props["item_width"].as<float>();
      }
      if (props["item_height"]) {
        ih = props["item_height"].as<float>();
      }
      tile->item_size(iw, ih);
    }
    ApplyWidthHeight(tile.get(), props);
    ApplyWeightCrossAlign(tile.get(), props, false);
    return tile;
  });

  Register("Tab", [](const YAML::Node& props, const HandlerMap&) {
    auto tab = std::make_unique<Tab>();
    if (props["selected"]) {
      tab->set_selected(props["selected"].as<int>());
    }
    if (props["header_height"]) {
      tab->header_height(props["header_height"].as<float>());
    }
    if (props["headers"] && props["headers"].IsSequence()) {
      std::vector<std::wstring> titles;
      for (const auto& h : props["headers"]) {
        titles.push_back(Utf8ToWide(h.as<std::string>()));
      }
      tab->set_headers(std::move(titles));
    }
    ApplyWidthHeight(tab.get(), props);
    ApplyWeightCrossAlign(tab.get(), props, false);
    return tab;
  });

  Register("Absolute", [](const YAML::Node& props, const HandlerMap&) {
    auto abs = std::make_unique<Absolute>();
    ApplyWidthHeight(abs.get(), props);
    ApplyWeightCrossAlign(abs.get(), props, false);
    return abs;
  });

  Register("Label", [](const YAML::Node& props, const HandlerMap&) {
    auto label = std::make_unique<Label>();
    if (props["text"]) {
      label->text(Utf8ToWide(props["text"].as<std::string>()));
    }
    if (props["font_size"]) {
      label->font_size(props["font_size"].as<float>());
    }
    if (props["align"]) {
      label->align(ParseTextAlign(props["align"].as<std::string>()));
    }
    if (props["preferred_height"]) {
      label->preferred_height(props["preferred_height"].as<float>());
    }
    ApplyWidthHeight(label.get(), props);
    ApplyWeightCrossAlign(label.get(), props, true);
    return label;
  });

  Register("Button", [](const YAML::Node& props, const HandlerMap& handlers) {
    auto btn = std::make_unique<Button>();
    if (props["text"]) {
      btn->text(Utf8ToWide(props["text"].as<std::string>()));
    }
    if (props["font_size"]) {
      btn->font_size(props["font_size"].as<float>());
    }
    ApplyButtonChrome(btn.get(), props);
    ApplyWidthHeight(btn.get(), props);
    ApplyWeightCrossAlign(btn.get(), props, false);
    BindOnClick(btn.get(), props, handlers);
    return btn;
  });

  Register("UserControl", [](const YAML::Node& props, const HandlerMap&) {
    auto view = std::make_unique<UserControl>();
    ApplyWidthHeight(view.get(), props);
    ApplyWeightCrossAlign(view.get(), props, false);
    return view;
  });

  Register("TextField", [](const YAML::Node& props, const HandlerMap&) {
    auto field = std::make_unique<TextField>();
    if (props["text"]) {
      field->text(Utf8ToWide(props["text"].as<std::string>()));
    }
    if (props["placeholder"]) {
      field->placeholder(Utf8ToWide(props["placeholder"].as<std::string>()));
    }
    if (props["password"]) {
      field->password(props["password"].as<bool>());
    }
    if (props["font_size"]) {
      field->font_size(props["font_size"].as<float>());
    }
    ApplyWidthHeight(field.get(), props);
    ApplyWeightCrossAlign(field.get(), props, false);
    return field;
  });

  Register("ImageView", [](const YAML::Node& props, const HandlerMap&) {
    auto image = std::make_unique<ImageView>();
    if (props["path"]) {
      image->LoadFromFile(Utf8ToWide(props["path"].as<std::string>()));
    }
    ApplyWidthHeight(image.get(), props);
    ApplyWeightCrossAlign(image.get(), props, false);
    return image;
  });

  Register("ImageButton",
           [](const YAML::Node& props, const HandlerMap& handlers) {
             auto btn = std::make_unique<ImageButton>();
             ApplyWidthHeight(btn.get(), props);
             ApplyWeightCrossAlign(btn.get(), props, false);
             BindOnClick(btn.get(), props, handlers);
             return btn;
           });

  Register("Checkbox", [](const YAML::Node& props, const HandlerMap&) {
    auto cb = std::make_unique<Checkbox>();
    if (props["text"]) {
      cb->text(Utf8ToWide(props["text"].as<std::string>()));
    }
    if (props["font_size"]) {
      cb->font_size(props["font_size"].as<float>());
    }
    if (props["checked"]) {
      cb->checked(props["checked"].as<bool>());
    }
    ApplyWidthHeight(cb.get(), props);
    ApplyWeightCrossAlign(cb.get(), props, false);
    return cb;
  });

  Register("Radio", [](const YAML::Node& props, const HandlerMap&) {
    auto radio = std::make_unique<Radio>();
    if (props["text"]) {
      radio->text(Utf8ToWide(props["text"].as<std::string>()));
    }
    if (props["font_size"]) {
      radio->font_size(props["font_size"].as<float>());
    }
    if (props["group_id"]) {
      radio->group_id(props["group_id"].as<int>());
    }
    if (props["checked"]) {
      radio->checked(props["checked"].as<bool>());
    }
    ApplyWidthHeight(radio.get(), props);
    ApplyWeightCrossAlign(radio.get(), props, false);
    return radio;
  });

  Register("Switch", [](const YAML::Node& props, const HandlerMap&) {
    auto sw = std::make_unique<Switch>();
    if (props["text"]) {
      sw->text(Utf8ToWide(props["text"].as<std::string>()));
    }
    if (props["font_size"]) {
      sw->font_size(props["font_size"].as<float>());
    }
    // Prefer "value" / "is_on"; also accept "on" (YAML bool key).
    if (props["value"]) {
      sw->on(props["value"].as<bool>());
    } else if (props["is_on"]) {
      sw->on(props["is_on"].as<bool>());
    } else if (props["on"] && props["on"].IsScalar()) {
      sw->on(props["on"].as<bool>());
    }
    ApplyWidthHeight(sw.get(), props);
    ApplyWeightCrossAlign(sw.get(), props, false);
    return sw;
  });

  Register("ScrollView", [](const YAML::Node& props, const HandlerMap&) {
    auto scroll = std::make_unique<ScrollView>();
    scroll->fill_width();
    ApplyWidthHeight(scroll.get(), props);
    ApplyWeightCrossAlign(scroll.get(), props, false);
    return scroll;
  });

  Register("Submenu", [](const YAML::Node& props, const HandlerMap&) {
    auto sm = std::make_unique<Submenu>();
    if (props["text"]) {
      sm->text(Utf8ToWide(props["text"].as<std::string>()));
    }
    if (props["open_on_hover"]) {
      sm->open_on_hover(props["open_on_hover"].as<bool>());
    }
    ApplySubmenuChrome(sm.get(), props);
    ApplyWidthHeight(sm.get(), props);
    ApplyWeightCrossAlign(sm.get(), props, false);
    return sm;
  });

  Register("ListView", [](const YAML::Node& props, const HandlerMap&) {
    auto list = std::make_unique<ListView>();
    if (props["font_size"]) {
      list->font_size(props["font_size"].as<float>());
    }
    if (props["items"] && props["items"].IsSequence()) {
      for (const auto& item : props["items"]) {
        list->AddItem(Utf8ToWide(item.as<std::string>()));
      }
    }
    if (props["selected"]) {
      list->set_selected_index(props["selected"].as<int>());
    }
    ApplyWidthHeight(list.get(), props);
    ApplyWeightCrossAlign(list.get(), props, false);
    return list;
  });

  Register("SplitView", [](const YAML::Node& props, const HandlerMap&) {
    auto split = std::make_unique<SplitView>();
    split->fill_width();
    ApplyWidthHeight(split.get(), props);
    ApplyWeightCrossAlign(split.get(), props, false);
    if (props["ratio"]) {
      split->set_ratio(props["ratio"].as<float>());
    }
    return split;
  });

  Register("ProgressBar", [](const YAML::Node& props, const HandlerMap&) {
    auto bar = std::make_unique<ProgressBar>();
    if (props["value"]) {
      bar->value(props["value"].as<float>());
    }
    if (props["indeterminate"]) {
      bar->indeterminate(props["indeterminate"].as<bool>());
    }
    ApplyWidthHeight(bar.get(), props);
    ApplyWeightCrossAlign(bar.get(), props, false);
    return bar;
  });

  Register("Slider", [](const YAML::Node& props, const HandlerMap&) {
    auto slider = std::make_unique<Slider>();
    if (props["value"]) {
      slider->value(props["value"].as<float>());
    }
    if (props["step"]) {
      slider->step(props["step"].as<float>());
    }
    if (props["tick_count"]) {
      slider->tick_count(props["tick_count"].as<int>());
    }
    if (props["orientation"]) {
      const auto o = props["orientation"].as<std::string>();
      if (o == "vertical" || o == "v") {
        slider->orientation(SliderOrientation::Vertical);
      } else {
        slider->orientation(SliderOrientation::Horizontal);
      }
    }
    ApplyWidthHeight(slider.get(), props);
    ApplyWeightCrossAlign(slider.get(), props, false);
    return slider;
  });

  Register("Combo", [](const YAML::Node& props, const HandlerMap&) {
    auto combo = std::make_unique<Combo>();
    if (props["font_size"]) {
      combo->font_size(props["font_size"].as<float>());
    }
    if (props["editable"]) {
      combo->editable(props["editable"].as<bool>());
    }
    if (props["multi"]) {
      combo->multi(props["multi"].as<bool>());
    }
    if (props["items"] && props["items"].IsSequence()) {
      std::vector<std::wstring> items;
      for (const auto& it : props["items"]) {
        items.push_back(Utf8ToWide(it.as<std::string>()));
      }
      combo->items(std::move(items));
    }
    if (props["selected"]) {
      if (props["selected"].IsSequence()) {
        std::vector<int> idxs;
        for (const auto& it : props["selected"]) {
          idxs.push_back(it.as<int>());
        }
        combo->selected_indices(std::move(idxs));
      } else {
        combo->selected(props["selected"].as<int>());
      }
    }
    ApplyWidthHeight(combo.get(), props);
    ApplyWeightCrossAlign(combo.get(), props, false);
    return combo;
  });

  Register("TextArea", [](const YAML::Node& props, const HandlerMap&) {
    auto area = std::make_unique<TextArea>();
    if (props["text"]) {
      area->text(Utf8ToWide(props["text"].as<std::string>()));
    }
    if (props["placeholder"]) {
      area->placeholder(Utf8ToWide(props["placeholder"].as<std::string>()));
    }
    if (props["font_size"]) {
      area->font_size(props["font_size"].as<float>());
    }
    if (props["wrap"]) {
      area->wrap(props["wrap"].as<bool>());
    }
    ApplyWidthHeight(area.get(), props);
    ApplyWeightCrossAlign(area.get(), props, false);
    return area;
  });

  // Thin YAML: count of Text rows for smoke demos. Real apps use fluent callbacks.
  Register("VirtualList", [](const YAML::Node& props, const HandlerMap&) {
    auto list = std::make_unique<VirtualList>();
    int count = 100;
    if (props["count"]) {
      count = std::max(0, props["count"].as<int>());
    }
    if (props["font_size"]) {
      list->font_size(props["font_size"].as<float>());
    }
    ApplyListColumnsHeader(list.get(), props);
    list->item_count([count]() { return count; });
    if (!list->columns().empty()) {
      list->item_cell_text([](int i, int col) {
        if (col == 0) {
          return L"行 " + std::to_wstring(i + 1);
        }
        if (col == 1) {
          return L"状态-" + std::to_wstring(i % 5);
        }
        if (col == 2) {
          return L"详情-" + std::to_wstring(i);
        }
        return L"#" + std::to_wstring(i);
      });
    } else {
      list->item_text([](int i) {
        return L"Virtual item " + std::to_wstring(i);
      });
    }
    ApplyWidthHeight(list.get(), props);
    ApplyWeightCrossAlign(list.get(), props, false);
    return list;
  });

  // TreeView: nested YAML `nodes:` with text / expanded / children / lazy / checked.
  Register("TreeView", [](const YAML::Node& props, const HandlerMap&) {
    auto tree = std::make_unique<TreeView>();
    if (props["font_size"]) {
      tree->font_size(props["font_size"].as<float>());
    }
    if (props["row_height"]) {
      tree->row_height(props["row_height"].as<float>());
    }
    if (props["indent"]) {
      tree->indent(props["indent"].as<float>());
    }
    if (props["checkable"]) {
      tree->checkable(props["checkable"].as<bool>());
    }
    if (props["check_cascade"]) {
      tree->check_cascade(props["check_cascade"].as<bool>());
    }
    std::function<void(int parent, const YAML::Node& seq)> load;
    load = [&](int parent, const YAML::Node& seq) {
      if (!seq || !seq.IsSequence()) {
        return;
      }
      for (const auto& item : seq) {
        std::wstring text = L"node";
        bool expanded = false;
        bool lazy = false;
        bool checked = false;
        if (item.IsMap()) {
          if (item["text"]) {
            text = Utf8ToWide(item["text"].as<std::string>());
          }
          if (item["expanded"]) {
            expanded = item["expanded"].as<bool>();
          }
          if (item["lazy"]) {
            lazy = item["lazy"].as<bool>();
          }
          if (item["checked"]) {
            checked = item["checked"].as<bool>();
          }
          const int id = tree->AddNode(parent, std::move(text), expanded);
          if (lazy) {
            tree->set_lazy(id, true);
          }
          if (checked) {
            tree->set_checked(id, TreeCheckState::Checked, false);
          }
          if (item["children"]) {
            load(id, item["children"]);
          }
        } else {
          tree->AddNode(parent, Utf8ToWide(item.as<std::string>()), false);
        }
      }
    };
    if (props["nodes"]) {
      load(-1, props["nodes"]);
    }
    ApplyWidthHeight(tree.get(), props);
    ApplyWeightCrossAlign(tree.get(), props, false);
    return tree;
  });

  // ItemList: count + optional item_template (row widget tree); bind/paint in code.
  Register("ItemList", [this](const YAML::Node& props, const HandlerMap& handlers) {
    auto list = std::make_unique<ItemList>();
    int count = 0;
    if (props["count"]) {
      count = std::max(0, props["count"].as<int>());
    }
    if (props["row_height"]) {
      list->row_height(props["row_height"].as<float>());
    }
    if (props["overscan"]) {
      list->overscan(props["overscan"].as<int>());
    }
    if (props["row_padding"]) {
      list->row_padding(props["row_padding"].as<float>());
    }
    ApplyListColumnsHeader(list.get(), props);
    if (props["item_template"]) {
      // Clone: YAML::Node is a view into the Load() document; that document is
      // destroyed when CreateFromYaml* returns. Deferring LoadYamlNode without
      // Clone (and without keeping ViewFactory alive) AVs on first SyncVisibleRows.
      const YAML::Node tmpl = YAML::Clone(props["item_template"]);
      list->item_template_factory([tmpl, handlers]() {
        ViewFactory factory;
        return LoadYamlNode(tmpl, factory, handlers);
      });
    }
    for (int i = 0; i < count; ++i) {
      list->AddItem();
    }
    if (props["selected"]) {
      list->set_selected_index(props["selected"].as<int>(), false);
    }
    ApplyWidthHeight(list.get(), props);
    ApplyWeightCrossAlign(list.get(), props, false);
    return list;
  });
}

std::unique_ptr<Node> ViewFactory::CreateFromYamlFile(
    const std::string& path, const HandlerMap& handlers) const {
  return LoadYamlFile(path, *this, handlers);
}

std::unique_ptr<Node> ViewFactory::CreateFromYamlString(
    const std::string& yaml, const HandlerMap& handlers) const {
  return LoadYamlString(yaml, *this, handlers);
}

std::unique_ptr<Node> ViewFactory::CreateFromYaml(
    const std::string& path_or_yaml, const HandlerMap& handlers) const {
  namespace fs = std::filesystem;
  std::error_code ec;
  const fs::path as_path(path_or_yaml);
  if (fs::exists(as_path, ec) && fs::is_regular_file(as_path, ec)) {
    return CreateFromYamlFile(path_or_yaml, handlers);
  }
  return CreateFromYamlString(path_or_yaml, handlers);
}

std::string ViewFactory::DumpTree(const Node* root) {
  std::ostringstream out;
  DumpTreeRec(root, 0, out);
  return out.str();
}

}  // namespace auralite::ui
