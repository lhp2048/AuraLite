#include "auralite/ui/factory.h"

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

TextAlign ParseAlign(const std::string& s) {
  if (s == "center" || s == "Center") {
    return TextAlign::Center;
  }
  if (s == "right" || s == "Right") {
    return TextAlign::Right;
  }
  return TextAlign::Left;
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
    return col;
  });

  Register("Row", [](const YAML::Node& props, const HandlerMap&) {
    auto row = std::make_unique<Row>();
    ApplyPaddingRow(row.get(), props);
    if (props["spacing"]) {
      row->spacing(props["spacing"].as<float>());
    }
    return row;
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
      label->align(ParseAlign(props["align"].as<std::string>()));
    }
    if (props["preferred_height"]) {
      label->preferred_height(props["preferred_height"].as<float>());
    }
    ApplyWidthHeight(label.get(), props);
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
    return field;
  });

  Register("ImageView", [](const YAML::Node& props, const HandlerMap&) {
    auto image = std::make_unique<ImageView>();
    if (props["path"]) {
      image->LoadFromFile(Utf8ToWide(props["path"].as<std::string>()));
    }
    ApplyWidthHeight(image.get(), props);
    return image;
  });

  Register("ImageButton",
           [](const YAML::Node& props, const HandlerMap& handlers) {
             auto btn = std::make_unique<ImageButton>();
             ApplyWidthHeight(btn.get(), props);
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
    return sw;
  });

  Register("ScrollView", [](const YAML::Node& props, const HandlerMap&) {
    auto scroll = std::make_unique<ScrollView>();
    scroll->fill_width();
    ApplyWidthHeight(scroll.get(), props);
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
    return list;
  });

  Register("SplitView", [](const YAML::Node& props, const HandlerMap&) {
    auto split = std::make_unique<SplitView>();
    split->fill_width();
    ApplyWidthHeight(split.get(), props);
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
