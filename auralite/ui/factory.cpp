#include "auralite/ui/factory.h"

#include "auralite/ui/absolute.h"
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
#include "auralite/ui/tab.h"
#include "auralite/ui/text_field.h"
#include "auralite/ui/tile.h"
#include "auralite/ui/yaml_loader.h"

#include <yaml-cpp/yaml.h>

#include <filesystem>
#include <sstream>
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
  if (dynamic_cast<const ListView*>(n)) {
    return "ListView";
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
    ApplyWidthHeight(btn.get(), props);
    ApplyWeightCrossAlign(btn.get(), props, false);
    BindOnClick(btn.get(), props, handlers);
    return btn;
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
