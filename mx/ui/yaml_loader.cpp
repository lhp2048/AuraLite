#include "mx/ui/yaml_loader.h"

#include "mx/ui/factory.h"
#include "mx/ui/scroll_view.h"
#include "mx/ui/split_view.h"
#include "mx/ui/submenu.h"
#include "mx/ui/theme.h"
#include "mx/ui/theme_yaml.h"

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace mx::ui {
namespace {

std::string ReadFileUtf8(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("YAML file not found: " + path);
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

std::pair<std::string, YAML::Node> UnwrapTypedNode(const YAML::Node& node) {
  if (!node || !node.IsMap() || node.size() != 1) {
    throw std::runtime_error(
        "YAML node must be a single-key map (Type: props)");
  }
  const auto it = node.begin();
  return {it->first.as<std::string>(), it->second};
}

void AttachChildren(Node* parent, const YAML::Node& props,
                    const ViewFactory& factory, const HandlerMap& handlers) {
  if (!parent || !props["children"] || !props["children"].IsSequence()) {
    return;
  }
  for (const auto& child : props["children"]) {
    parent->AddChild(LoadYamlNode(child, factory, handlers));
  }
}

void AttachScrollContent(ScrollView* scroll, const YAML::Node& props,
                         const ViewFactory& factory,
                         const HandlerMap& handlers) {
  if (!scroll || !props["content"]) {
    return;
  }
  scroll->set_content(LoadYamlNode(props["content"], factory, handlers));
}

void AttachSubmenuContent(Submenu* sm, const YAML::Node& props,
                          const ViewFactory& factory,
                          const HandlerMap& handlers) {
  if (!sm || !props["content"]) {
    return;
  }
  sm->content(LoadYamlNode(props["content"], factory, handlers));
}

void AttachSplitPanes(SplitView* split, const YAML::Node& props,
                      const ViewFactory& factory, const HandlerMap& handlers) {
  if (!split) {
    return;
  }
  if (props["leading"]) {
    split->set_leading(LoadYamlNode(props["leading"], factory, handlers));
  }
  if (props["trailing"]) {
    split->set_trailing(LoadYamlNode(props["trailing"], factory, handlers));
  }
}

bool ParseAccRole(const std::string& raw, AccRole* out) {
  if (!out) {
    return false;
  }
  std::string k = raw;
  for (char& ch : k) {
    if (ch >= 'A' && ch <= 'Z') {
      ch = static_cast<char>(ch - 'A' + 'a');
    }
  }
  if (k == "ignore") {
    *out = AccRole::Ignore;
  } else if (k == "group") {
    *out = AccRole::Group;
  } else if (k == "button") {
    *out = AccRole::Button;
  } else if (k == "text") {
    *out = AccRole::Text;
  } else if (k == "edit") {
    *out = AccRole::Edit;
  } else if (k == "checkbox") {
    *out = AccRole::CheckBox;
  } else if (k == "radiobutton" || k == "radio") {
    *out = AccRole::RadioButton;
  } else if (k == "combobox" || k == "combo") {
    *out = AccRole::ComboBox;
  } else if (k == "menuitem" || k == "menu_item") {
    *out = AccRole::MenuItem;
  } else if (k == "slider") {
    *out = AccRole::Slider;
  } else if (k == "progressbar" || k == "progress") {
    *out = AccRole::ProgressBar;
  } else if (k == "tab") {
    *out = AccRole::Tab;
  } else if (k == "list") {
    *out = AccRole::List;
  } else if (k == "tree") {
    *out = AccRole::Tree;
  } else if (k == "spinner" || k == "spinbox" || k == "spin") {
    *out = AccRole::Spinner;
  } else if (k == "menubar" || k == "menu_bar") {
    *out = AccRole::MenuBar;
  } else if (k == "statusbar" || k == "status_bar") {
    *out = AccRole::StatusBar;
  } else {
    return false;
  }
  return true;
}

void ParseWindowYaml(const YAML::Node& n, WindowYaml* out) {
  if (!out || !n || !n.IsMap()) {
    return;
  }
  out->present = true;
  if (n["title"] && n["title"].IsScalar()) {
    out->title = Utf8ToWide(n["title"].as<std::string>());
  }
  if (n["width"]) {
    out->width = n["width"].as<int>();
  }
  if (n["height"]) {
    out->height = n["height"].as<int>();
  }
  if (n["kind"] && n["kind"].IsScalar()) {
    std::string k = n["kind"].as<std::string>();
    for (char& ch : k) {
      if (ch >= 'A' && ch <= 'Z') {
        ch = static_cast<char>(ch - 'A' + 'a');
      }
    }
    if (k == "dialog") {
      out->kind = WindowYaml::Kind::Dialog;
      out->options = Window::WindowOptions::Dialog();
    } else if (k == "popup") {
      out->kind = WindowYaml::Kind::Popup;
    } else if (k == "main" || k == "window") {
      out->kind = WindowYaml::Kind::Main;
    }
  }
  if (n["caption"]) {
    out->options.caption = n["caption"].as<bool>();
  }
  if (n["quit_on_close"]) {
    out->options.quit_on_close = n["quit_on_close"].as<bool>();
  }
  if (n["topmost"]) {
    out->options.topmost = n["topmost"].as<bool>();
  }
  if (n["center_on_owner"]) {
    out->options.center_on_owner = n["center_on_owner"].as<bool>();
  }
  if (n["corner_radius"]) {
    out->options.corner_radius = n["corner_radius"].as<float>();
    out->has_corner_radius = true;
  }
  if (n["border_width"]) {
    out->options.border_width = n["border_width"].as<float>();
    out->has_border_width = true;
  }
  if (n["resizable"]) {
    out->options.resizable = n["resizable"].as<bool>();
  }
  if (n["min_width"]) {
    out->options.min_width = n["min_width"].as<int>();
  }
  if (n["min_height"]) {
    out->options.min_height = n["min_height"].as<int>();
  }
  if (n["theme"] && n["theme"].IsScalar()) {
    out->theme = n["theme"].as<std::string>();
  }
}

YAML::Node StripRootMetaKeys(const YAML::Node& root) {
  YAML::Node peeled(YAML::NodeType::Map);
  for (auto it = root.begin(); it != root.end(); ++it) {
    const std::string k = it->first.as<std::string>();
    if (k != "theme" && k != "window") {
      peeled[it->first] = it->second;
    }
  }
  return peeled;
}

// Root may be a typed node, or a wrapper map with optional `theme:` / `window:`.
// `root["theme"]` is safe here: `root` is const (yaml-cpp const [] does
// not insert a missing key, which would break UnwrapTypedNode).
YAML::Node PeelRootMeta(const YAML::Node& root, WindowYaml* window_out) {
  if (!root || !root.IsMap()) {
    return root;
  }
  if (root["theme"] && root["theme"].IsScalar()) {
    Theme::SetActive(root["theme"].as<std::string>());
  }
  if (root["window"] && root["window"].IsMap()) {
    WindowYaml parsed;
    ParseWindowYaml(root["window"], &parsed);
    if (window_out) {
      *window_out = parsed;
    }
  }
  if (!root["theme"] && !root["window"]) {
    return root;
  }
  if (root.size() <= 1) {
    return root;
  }
  YAML::Node peeled = StripRootMetaKeys(root);
  if (!peeled || peeled.size() == 0) {
    return root;
  }
  return peeled;
}

}  // namespace

std::wstring Utf8ToWide(const std::string& utf8) {
  if (utf8.empty()) {
    return {};
  }
  const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                    static_cast<int>(utf8.size()), nullptr, 0);
  if (n <= 0) {
    return {};
  }
  std::wstring out(static_cast<size_t>(n), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()),
                      out.data(), n);
  return out;
}

std::unique_ptr<Node> LoadYamlNode(const YAML::Node& node,
                                   const ViewFactory& factory,
                                   const HandlerMap& handlers) {
  auto [type, props] = UnwrapTypedNode(node);
  if (!props || props.IsNull()) {
    props = YAML::Node(YAML::NodeType::Map);
  }
  if (!props.IsMap()) {
    throw std::runtime_error("YAML props for " + type + " must be a map");
  }

  std::unique_ptr<Node> built = factory.Build(type, props, handlers);
  if (!built) {
    throw std::runtime_error("Unknown or failed YAML type: " + type);
  }

  if (props["name"]) {
    built->set_name(props["name"].as<std::string>());
  }

  if (props["bg"]) {
    ColorF c;
    if (ParseColorHex(props["bg"].as<std::string>(), &c)) {
      built->bg(c);
    }
  }

  if (props["clip"]) {
    built->clip_children(props["clip"].as<bool>());
  }

  if (props["tooltip"]) {
    built->tooltip(Utf8ToWide(props["tooltip"].as<std::string>()));
  }

  if (props["animate"]) {
    built->animate(props["animate"].as<bool>());
  }

  if (props["draggable"]) {
    built->draggable(props["draggable"].as<bool>());
  }
  if (props["drag_data"]) {
    built->drag_data(Utf8ToWide(props["drag_data"].as<std::string>()));
  }
  if (props["drop_target"]) {
    built->drop_target(props["drop_target"].as<bool>());
  }

  if (props["acc_name"]) {
    built->acc_name(Utf8ToWide(props["acc_name"].as<std::string>()));
  }
  if (props["acc_role"] && props["acc_role"].IsScalar()) {
    AccRole role = AccRole::Ignore;
    if (ParseAccRole(props["acc_role"].as<std::string>(), &role)) {
      built->set_acc_role(role);
    }
  }

  // Absolute placement: anchors preferred; x/y as fallback origin.
  if (props["x"] || props["y"]) {
    const float x = props["x"] ? props["x"].as<float>() : 0.f;
    const float y = props["y"] ? props["y"].as<float>() : 0.f;
    built->set_pos(x, y);
  }
  if (props["left"]) {
    built->left(props["left"].as<float>());
  }
  if (props["top"]) {
    built->top(props["top"].as<float>());
  }
  if (props["right"]) {
    built->right(props["right"].as<float>());
  }
  if (props["bottom"]) {
    built->bottom(props["bottom"].as<float>());
  }

  if (type == "Column" || type == "Row" || type == "TitleBar" ||
      type == "Tile" || type == "Tab" || type == "Absolute") {
    AttachChildren(built.get(), props, factory, handlers);
  } else if (type == "ScrollView") {
    if (auto* scroll = dynamic_cast<ScrollView*>(built.get())) {
      AttachScrollContent(scroll, props, factory, handlers);
    }
  } else if (type == "Submenu") {
    if (auto* sm = dynamic_cast<Submenu*>(built.get())) {
      AttachSubmenuContent(sm, props, factory, handlers);
    }
  } else if (type == "SplitView") {
    if (auto* split = dynamic_cast<SplitView*>(built.get())) {
      AttachSplitPanes(split, props, factory, handlers);
    }
  }

  return built;
}

std::unique_ptr<Node> LoadYamlString(const std::string& yaml,
                                     const ViewFactory& factory,
                                     const HandlerMap& handlers,
                                     WindowYaml* window_out) {
  const YAML::Node root = YAML::Load(yaml);
  return LoadYamlNode(PeelRootMeta(root, window_out), factory, handlers);
}

std::unique_ptr<Node> LoadYamlFile(const std::string& path,
                                   const ViewFactory& factory,
                                   const HandlerMap& handlers,
                                   WindowYaml* window_out) {
  return LoadYamlString(ReadFileUtf8(path), factory, handlers, window_out);
}

}  // namespace mx::ui
