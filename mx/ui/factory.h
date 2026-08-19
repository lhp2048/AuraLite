#pragma once

#include "mx/ui/node.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace YAML {
class Node;
}

namespace mx::ui {

using HandlerMap = std::unordered_map<std::string, std::function<void()>>;

// Builds a typed Node from YAML property map (the map under the type key).
// Structural children (children / content / leading / trailing) are attached
// by the YAML loader after the builder returns.
using NodeBuilder =
    std::function<std::unique_ptr<Node>(const YAML::Node& props,
                                        const HandlerMap& handlers)>;

struct WindowYaml;

class ViewFactory {
 public:
  ViewFactory();

  void Register(const std::string& type, NodeBuilder builder);
  void RegisterBuiltinTypes();

  bool HasType(const std::string& type) const;
  std::unique_ptr<Node> Build(const std::string& type, const YAML::Node& props,
                              const HandlerMap& handlers) const;

  // If |path_or_yaml| names an existing file, load it; otherwise parse as YAML.
  std::unique_ptr<Node> CreateFromYaml(const std::string& path_or_yaml,
                                       const HandlerMap& handlers,
                                       WindowYaml* window_out = nullptr) const;
  std::unique_ptr<Node> CreateFromYamlFile(const std::string& path,
                                           const HandlerMap& handlers,
                                           WindowYaml* window_out = nullptr) const;
  std::unique_ptr<Node> CreateFromYamlString(const std::string& yaml,
                                             const HandlerMap& handlers,
                                             WindowYaml* window_out = nullptr) const;

  // Debug: type names + key text props, indented by depth.
  static std::string DumpTree(const Node* root);

 private:
  std::unordered_map<std::string, NodeBuilder> builders_;
};

}  // namespace mx::ui
