#pragma once

#include "mx/ui/node.h"
#include "mx/ui/vertical_scrollbar.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace mx::ui {

enum class TreeCheckState { Unchecked = 0, Checked = 1, Partial = 2 };

// Self-scrolling tree: expand/collapse, indent, optional check + lazy children.
class TreeView : public Node {
 public:
  using SelectionHandler = std::function<void(int id)>;
  using ExpandHandler = std::function<void(int id, bool expanded)>;
  using CheckHandler = std::function<void(int id, TreeCheckState state)>;
  using LoadChildrenHandler = std::function<void(int id)>;

  TreeView();

  AccRole acc_role() const override;

  void Clear();
  // Returns node id (>=0). parent_id < 0 => root.
  int AddNode(int parent_id, std::wstring text, bool expanded = false);
  int AddRoot(std::wstring text, bool expanded = false) {
    return AddNode(-1, std::move(text), expanded);
  }
  int AddChild(int parent_id, std::wstring text, bool expanded = false) {
    return AddNode(parent_id, std::move(text), expanded);
  }

  int node_count() const { return static_cast<int>(nodes_.size()); }
  bool valid_id(int id) const {
    return id >= 0 && id < static_cast<int>(nodes_.size());
  }
  const std::wstring& text(int id) const;
  void set_text(int id, std::wstring text);

  void set_expanded(int id, bool expanded, bool notify = true);
  bool expanded(int id) const;
  void ExpandAll();
  void CollapseAll();

  void set_selected_id(int id, bool notify = true);
  int selected_id() const { return selected_id_; }

  TreeView& checkable(bool v);
  bool checkable() const { return checkable_; }
  TreeView& check_cascade(bool v);
  bool check_cascade() const { return check_cascade_; }
  void set_checked(int id, TreeCheckState state, bool notify = true);
  TreeCheckState checked(int id) const;
  TreeView& on_check_changed(CheckHandler handler);

  // Lazy: show expander before children exist. First expand -> on_load_children.
  void set_lazy(int id, bool lazy);
  bool lazy(int id) const;
  bool children_loaded(int id) const;
  bool loading(int id) const;
  void NotifyChildrenLoaded(int id);
  TreeView& on_load_children(LoadChildrenHandler handler);

  TreeView& on_selection_changed(SelectionHandler handler);
  TreeView& on_expanded_changed(ExpandHandler handler);
  TreeView& font_size(float size);
  TreeView& row_height(float h);
  TreeView& indent(float px);

  void RebuildVisible();
  void EnsureVisibleId(int id);

  SizeF Measure(float max_w, float max_h) override;
  void Layout(const RectF& final_rect) override;
  void Paint(mx::Canvas& canvas) override;

  bool WantsMouseWheel() const override { return true; }
  void OnMouseWheel(const MouseEvent& e) override;
  void OnMouseDown(const MouseEvent& e) override;
  void OnMouseMove(const MouseEvent& e) override;
  void OnMouseUp(const MouseEvent& e) override;
  void OnMouseLeave(const MouseEvent& e) override;
  void OnKey(const KeyEvent& e) override;

 private:
  struct NodeData {
    int parent = -1;
    std::wstring text;
    bool expanded = false;
    bool lazy = false;
    bool children_loaded = true;
    bool loading = false;
    TreeCheckState check = TreeCheckState::Unchecked;
    std::vector<int> children;
  };
  struct VisibleRow {
    int id = -1;
    int depth = 0;
  };

  static constexpr float kPadX = 6.f;
  static constexpr float kTwistSize = 14.f;
  static constexpr float kCheckSize = 14.f;

  void Flatten(int id, int depth);
  float ContentHeight() const;
  float ViewportWidth() const;
  float ViewportHeight() const;
  float MaxScrollOffset() const;
  bool NeedsScrollbar() const;
  void ClampScroll();
  void set_scroll_offset(float y);
  void SyncVScrollBar();
  void VisibleRange(int* first, int* last) const;
  int RowAtPoint(float x, float y) const;
  RectF RowRect(int row_index) const;
  RectF TwistRect(const RectF& row, int depth) const;
  RectF CheckRect(const RectF& row, int depth) const;
  bool HitTwist(int row_index, float x, float y) const;
  bool HitCheck(int row_index, float x, float y) const;
  bool HasChildren(int id) const;
  bool CanExpand(int id) const;
  bool NeedsLazyLoad(int id) const;
  void RequestLazyLoad(int id);
  int VisibleIndexOfId(int id) const;
  void CommitSelection();
  void SetExpandedRecursive(int id, bool expanded);
  void SetCheckRecursive(int id, TreeCheckState state);
  void UpdateAncestorsCheck(int id);
  TreeCheckState AggregateChildrenCheck(int id) const;
  void ToggleCheck(int id);
  void PaintCheckBox(mx::Canvas& canvas, const RectF& box,
                     TreeCheckState state, bool selected) const;

  std::vector<NodeData> nodes_;
  std::vector<int> roots_;
  std::vector<VisibleRow> visible_;
  bool visible_dirty_ = true;

  std::optional<float> font_size_;
  float row_h_ = 28.f;
  float indent_ = 18.f;
  float scroll_y_ = 0.f;
  int selected_id_ = -1;
  int hover_row_ = -1;
  VerticalScrollbar vscroll_;
  bool checkable_ = false;
  bool check_cascade_ = true;

  SelectionHandler on_selection_;
  ExpandHandler on_expand_;
  CheckHandler on_check_;
  LoadChildrenHandler on_load_;
};

}  // namespace mx::ui
