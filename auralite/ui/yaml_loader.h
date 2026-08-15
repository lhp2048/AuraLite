#pragma once

#include "auralite/ui/factory.h"

#include <memory>
#include <string>

namespace YAML {
class Node;
}

namespace auralite::ui {

class ViewFactory;

// Mini YAML subset (Spec §3.4): type-as-key maps, children lists,
// on_click handler names, UTF-8 → wide UI strings.
//
// LoadYamlFile / LoadYamlString accept either:
//   1) A single-key typed node: `ScrollView: { ... }`
//   2) A root map that wraps that node plus optional scalar `theme:`:
//        theme: dark
//        ScrollView: { ... }
//      `theme` calls Theme::SetActive before the tree is built, then is
//      peeled so LoadYamlNode still sees exactly one type key.
std::unique_ptr<Node> LoadYamlNode(const YAML::Node& node,
                                   const ViewFactory& factory,
                                   const HandlerMap& handlers);

std::unique_ptr<Node> LoadYamlFile(const std::string& path,
                                   const ViewFactory& factory,
                                   const HandlerMap& handlers);

std::unique_ptr<Node> LoadYamlString(const std::string& yaml,
                                     const ViewFactory& factory,
                                     const HandlerMap& handlers);

std::wstring Utf8ToWide(const std::string& utf8);

}  // namespace auralite::ui
