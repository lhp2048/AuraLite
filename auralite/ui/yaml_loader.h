#pragma once

#include "auralite/ui/factory.h"
#include "auralite/ui/window.h"

#include <memory>
#include <string>

namespace YAML {
class Node;
}

namespace auralite::ui {

class ViewFactory;

// Optional root `window:` map (peeled like `theme:`, not part of the Node tree).
struct WindowYaml {
  bool present = false;
  std::wstring title;
  int width = 0;
  int height = 0;
  enum class Kind { Unspecified, Main, Dialog, Popup };
  Kind kind = Kind::Unspecified;
  Window::WindowOptions options{};
  bool has_corner_radius = false;
  bool has_border_width = false;
  std::string theme;

  int width_or(int fallback) const { return width > 0 ? width : fallback; }
  int height_or(int fallback) const { return height > 0 ? height : fallback; }
  const wchar_t* title_or(const wchar_t* fallback) const {
    return title.empty() ? fallback : title.c_str();
  }
  Window::WindowOptions create_options(HWND owner = nullptr) const {
    Window::WindowOptions o = options;
    if (owner) {
      o.owner = owner;
    }
    o.Normalize();
    return o;
  }
};

// Mini YAML subset (Spec §3.4): type-as-key maps, children lists,
// on_click handler names, UTF-8 → wide UI strings.
//
// LoadYamlFile / LoadYamlString accept either:
//   1) A single-key typed node: `ScrollView: { ... }`
//   2) A root map that wraps that node plus optional scalar `theme:`
//      and optional map `window:`:
//        theme: dark
//        window:
//          title: Dialog
//          width: 320
//          height: 180
//          kind: dialog
//          caption: false
//          theme: dark
//          corner_radius: 8
//          border_width: 1
//        Column: { ... }
//      Root `theme` calls Theme::SetActive (process) before the tree is built.
//      `window.theme` is per-window (Window::set_theme); empty inherits process.
//      `window` fills |window_out| if provided. Both keys are peeled so
//      LoadYamlNode still sees exactly one type key.
//      create_options() calls WindowOptions::Normalize: corner_radius /
//      border_width apply only when caption is false.
std::unique_ptr<Node> LoadYamlNode(const YAML::Node& node,
                                   const ViewFactory& factory,
                                   const HandlerMap& handlers);

std::unique_ptr<Node> LoadYamlFile(const std::string& path,
                                   const ViewFactory& factory,
                                   const HandlerMap& handlers,
                                   WindowYaml* window_out = nullptr);

std::unique_ptr<Node> LoadYamlString(const std::string& yaml,
                                     const ViewFactory& factory,
                                     const HandlerMap& handlers,
                                     WindowYaml* window_out = nullptr);

std::wstring Utf8ToWide(const std::string& utf8);

}  // namespace auralite::ui
