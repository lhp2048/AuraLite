#include "auralite/ui/yaml_loader.h"

#include "auralite/ui/factory.h"
#include "auralite/ui/scroll_view.h"
#include "auralite/ui/split_view.h"
#include "auralite/ui/submenu.h"
#include "auralite/ui/theme.h"
#include "auralite/ui/theme_yaml.h"

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace auralite::ui {
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

// Root may be a typed node, or a wrapper map with optional `theme:`.
// `root["theme"]` is safe here: `root` is const (yaml-cpp const [] does
// not insert a missing key, which would break UnwrapTypedNode).
YAML::Node PeelThemeAndActivate(const YAML::Node& root) {
  if (!root || !root.IsMap() || !root["theme"] || !root["theme"].IsScalar()) {
    return root;
  }
  Theme::SetActive(root["theme"].as<std::string>());
  if (root.size() <= 1) {
    return root;
  }
  YAML::Node peeled(YAML::NodeType::Map);
  for (auto it = root.begin(); it != root.end(); ++it) {
    if (it->first.as<std::string>() != "theme") {
      peeled[it->first] = it->second;
    }
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

  if (type == "Column" || type == "Row" || type == "Tile" || type == "Tab" ||
      type == "Absolute") {
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
                                     const HandlerMap& handlers) {
  const YAML::Node root = YAML::Load(yaml);
  return LoadYamlNode(PeelThemeAndActivate(root), factory, handlers);
}

std::unique_ptr<Node> LoadYamlFile(const std::string& path,
                                   const ViewFactory& factory,
                                   const HandlerMap& handlers) {
  return LoadYamlString(ReadFileUtf8(path), factory, handlers);
}

}  // namespace auralite::ui
