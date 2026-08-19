#include "mx/ui/acc.h"

#include "mx/ui/node.h"

namespace mx::ui {

void CollectAccNodes(const Node* root, std::vector<const Node*>* out) {
  if (!root || !out || !root->visible()) {
    return;
  }
  if (root->AccIncluded()) {
    root->EnsureAccId();
    out->push_back(root);
  }
  for (const auto& child : root->children()) {
    CollectAccNodes(child.get(), out);
  }
}

void CollectAccNodes(Node* root, std::vector<Node*>* out) {
  if (!root || !out || !root->visible()) {
    return;
  }
  if (root->AccIncluded()) {
    root->EnsureAccId();
    out->push_back(root);
  }
  for (const auto& child : root->children()) {
    CollectAccNodes(child.get(), out);
  }
}

Node* FindAccNode(Node* root, int acc_id) {
  if (!root || acc_id == 0 || !root->visible()) {
    return nullptr;
  }
  if (root->acc_id() == acc_id) {
    return root->AccIncluded() ? root : nullptr;
  }
  for (auto& child : root->children()) {
    if (Node* hit = FindAccNode(child.get(), acc_id)) {
      return hit;
    }
  }
  return nullptr;
}

const Node* FindAccNode(const Node* root, int acc_id) {
  return FindAccNode(const_cast<Node*>(root), acc_id);
}

}  // namespace mx::ui
