#pragma once

#include <string>
#include <vector>

namespace mx::ui {

class Node;

enum class AccRole {
  Ignore = 0,
  Group,
  Button,
  Text,
  Edit,
  CheckBox,
  RadioButton,
  ComboBox,
  MenuItem,
  Slider,
  ProgressBar,
  Tab,
  List,
  Tree,
  Spinner,
  MenuBar,
  StatusBar,
};

struct AccState {
  bool focused = false;
  bool disabled = false;
  bool checked = false;
  bool password = false;
};

void CollectAccNodes(const Node* root, std::vector<const Node*>* out);
void CollectAccNodes(Node* root, std::vector<Node*>* out);
Node* FindAccNode(Node* root, int acc_id);
const Node* FindAccNode(const Node* root, int acc_id);

}  // namespace mx::ui
