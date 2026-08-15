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
