#include "auralite/ui/tree_view.h"

#include "auralite/ui/theme.h"

#include <algorithm>

namespace auralite::ui {

TreeView::TreeView() {
  set_focusable(true);
  fill_width();
  fixed_height(200.f);
}

void TreeView::Clear() {
  nodes_.clear();
  roots_.clear();
  visible_.clear();
  selected_id_ = -1;
  hover_row_ = -1;
  scroll_y_ = 0.f;
  visible_dirty_ = true;
}

int TreeView::AddNode(int parent_id, std::wstring text, bool expanded) {
  const int id = static_cast<int>(nodes_.size());
  NodeData n;
  n.parent = parent_id;
  n.text = std::move(text);
  n.expanded = expanded;
  nodes_.push_back(std::move(n));
  if (parent_id < 0) {
    roots_.push_back(id);
  } else if (valid_id(parent_id)) {
    nodes_[static_cast<size_t>(parent_id)].children.push_back(id);
    // Parent that receives children is no longer waiting on empty lazy load.
    if (nodes_[static_cast<size_t>(parent_id)].lazy) {
      nodes_[static_cast<size_t>(parent_id)].children_loaded = true;
    }
  } else {
    roots_.push_back(id);
    nodes_.back().parent = -1;
  }
  visible_dirty_ = true;
  return id;
}

const std::wstring& TreeView::text(int id) const {
  static const std::wstring kEmpty;
  if (!valid_id(id)) {
    return kEmpty;
  }
  return nodes_[static_cast<size_t>(id)].text;
}

void TreeView::set_text(int id, std::wstring text) {
  if (!valid_id(id)) {
    return;
  }
  nodes_[static_cast<size_t>(id)].text = std::move(text);
}

bool TreeView::HasChildren(int id) const {
  return valid_id(id) && !nodes_[static_cast<size_t>(id)].children.empty();
}

bool TreeView::NeedsLazyLoad(int id) const {
  if (!valid_id(id)) {
    return false;
  }
  const NodeData& n = nodes_[static_cast<size_t>(id)];
  return n.lazy && !n.children_loaded && !n.loading;
}

bool TreeView::CanExpand(int id) const {
  if (!valid_id(id)) {
    return false;
  }
  const NodeData& n = nodes_[static_cast<size_t>(id)];
  return HasChildren(id) || (n.lazy && !n.children_loaded) || n.loading;
}

void TreeView::RequestLazyLoad(int id) {
  if (!NeedsLazyLoad(id)) {
    return;
  }
  nodes_[static_cast<size_t>(id)].loading = true;
  if (on_load_) {
    on_load_(id);
  }
}

void TreeView::set_expanded(int id, bool expanded, bool notify) {
  if (!valid_id(id) || !CanExpand(id)) {
    return;
  }
  NodeData& n = nodes_[static_cast<size_t>(id)];
  if (expanded && NeedsLazyLoad(id)) {
    RequestLazyLoad(id);
  }
  if (n.expanded == expanded) {
    visible_dirty_ = true;
    return;
  }
  n.expanded = expanded;
  visible_dirty_ = true;
  if (notify && on_expand_) {
    on_expand_(id, expanded);
  }
}

bool TreeView::expanded(int id) const {
  return valid_id(id) && nodes_[static_cast<size_t>(id)].expanded;
}

void TreeView::SetExpandedRecursive(int id, bool expanded) {
  if (!valid_id(id)) {
    return;
  }
  if (CanExpand(id)) {
    if (expanded && NeedsLazyLoad(id)) {
      RequestLazyLoad(id);
    }
    nodes_[static_cast<size_t>(id)].expanded = expanded;
    for (int c : nodes_[static_cast<size_t>(id)].children) {
      SetExpandedRecursive(c, expanded);
    }
  }
}

void TreeView::ExpandAll() {
  for (int r : roots_) {
    SetExpandedRecursive(r, true);
  }
  visible_dirty_ = true;
}

void TreeView::CollapseAll() {
  for (int r : roots_) {
    SetExpandedRecursive(r, false);
  }
  visible_dirty_ = true;
}

TreeView& TreeView::checkable(bool v) {
  checkable_ = v;
  return *this;
}

TreeView& TreeView::check_cascade(bool v) {
  check_cascade_ = v;
  return *this;
}

TreeView& TreeView::on_check_changed(CheckHandler handler) {
  on_check_ = std::move(handler);
  return *this;
}

TreeCheckState TreeView::checked(int id) const {
  if (!valid_id(id)) {
    return TreeCheckState::Unchecked;
  }
  return nodes_[static_cast<size_t>(id)].check;
}

void TreeView::SetCheckRecursive(int id, TreeCheckState state) {
  if (!valid_id(id) || state == TreeCheckState::Partial) {
    return;
  }
  nodes_[static_cast<size_t>(id)].check = state;
  for (int c : nodes_[static_cast<size_t>(id)].children) {
    SetCheckRecursive(c, state);
  }
}

TreeCheckState TreeView::AggregateChildrenCheck(int id) const {
  if (!HasChildren(id)) {
    return nodes_[static_cast<size_t>(id)].check;
  }
  bool any_checked = false;
  bool any_unchecked = false;
  bool any_partial = false;
  for (int c : nodes_[static_cast<size_t>(id)].children) {
    const TreeCheckState s = nodes_[static_cast<size_t>(c)].check;
    if (s == TreeCheckState::Checked) {
      any_checked = true;
    } else if (s == TreeCheckState::Unchecked) {
      any_unchecked = true;
    } else {
      any_partial = true;
    }
  }
  if (any_partial || (any_checked && any_unchecked)) {
    return TreeCheckState::Partial;
  }
  if (any_checked) {
    return TreeCheckState::Checked;
  }
  return TreeCheckState::Unchecked;
}

void TreeView::UpdateAncestorsCheck(int id) {
  int p = valid_id(id) ? nodes_[static_cast<size_t>(id)].parent : -1;
  while (p >= 0) {
    nodes_[static_cast<size_t>(p)].check = AggregateChildrenCheck(p);
    p = nodes_[static_cast<size_t>(p)].parent;
  }
}

void TreeView::set_checked(int id, TreeCheckState state, bool notify) {
  if (!valid_id(id) || !checkable_) {
    return;
  }
  if (state == TreeCheckState::Partial) {
    nodes_[static_cast<size_t>(id)].check = state;
  } else if (check_cascade_) {
    SetCheckRecursive(id, state);
  } else {
    nodes_[static_cast<size_t>(id)].check = state;
  }
  if (check_cascade_) {
    UpdateAncestorsCheck(id);
  }
  if (notify && on_check_) {
    on_check_(id, nodes_[static_cast<size_t>(id)].check);
  }
}

void TreeView::ToggleCheck(int id) {
  if (!valid_id(id) || !checkable_) {
    return;
  }
  const TreeCheckState cur = nodes_[static_cast<size_t>(id)].check;
  const TreeCheckState next = (cur == TreeCheckState::Checked)
                                  ? TreeCheckState::Unchecked
                                  : TreeCheckState::Checked;
  set_checked(id, next, true);
}

void TreeView::set_lazy(int id, bool lazy) {
  if (!valid_id(id)) {
    return;
  }
  NodeData& n = nodes_[static_cast<size_t>(id)];
  n.lazy = lazy;
  if (lazy && n.children.empty()) {
    n.children_loaded = false;
  }
  if (!lazy) {
    n.children_loaded = true;
    n.loading = false;
  }
  visible_dirty_ = true;
}

bool TreeView::lazy(int id) const {
  return valid_id(id) && nodes_[static_cast<size_t>(id)].lazy;
}

bool TreeView::children_loaded(int id) const {
  return valid_id(id) && nodes_[static_cast<size_t>(id)].children_loaded;
}

bool TreeView::loading(int id) const {
  return valid_id(id) && nodes_[static_cast<size_t>(id)].loading;
}

void TreeView::NotifyChildrenLoaded(int id) {
  if (!valid_id(id)) {
    return;
  }
  NodeData& n = nodes_[static_cast<size_t>(id)];
  n.loading = false;
  n.children_loaded = true;
  n.lazy = n.lazy;  // keep flag; further expands use children
  visible_dirty_ = true;
  RebuildVisible();
}

TreeView& TreeView::on_load_children(LoadChildrenHandler handler) {
  on_load_ = std::move(handler);
  return *this;
}

TreeView& TreeView::on_selection_changed(SelectionHandler handler) {
  on_selection_ = std::move(handler);
  return *this;
}

TreeView& TreeView::on_expanded_changed(ExpandHandler handler) {
  on_expand_ = std::move(handler);
  return *this;
}

TreeView& TreeView::font_size(float size) {
  font_size_ = size;
  return *this;
}

TreeView& TreeView::row_height(float h) {
  row_h_ = std::max(16.f, h);
  return *this;
}

TreeView& TreeView::indent(float px) {
  indent_ = std::max(8.f, px);
  return *this;
}

void TreeView::Flatten(int id, int depth) {
  if (!valid_id(id)) {
    return;
  }
  visible_.push_back(VisibleRow{id, depth});
  const NodeData& n = nodes_[static_cast<size_t>(id)];
  if (n.expanded) {
    for (int c : n.children) {
      Flatten(c, depth + 1);
    }
  }
}

void TreeView::RebuildVisible() {
  visible_.clear();
  for (int r : roots_) {
    Flatten(r, 0);
  }
  visible_dirty_ = false;
  ClampScroll();
}

void TreeView::CommitSelection() {
  if (on_selection_ && selected_id_ >= 0) {
    on_selection_(selected_id_);
  }
}

void TreeView::set_selected_id(int id, bool notify) {
  if (id != -1 && !valid_id(id)) {
    return;
  }
  if (selected_id_ == id) {
    if (notify) {
      CommitSelection();
    }
    return;
  }
  selected_id_ = id;
  if (selected_id_ >= 0) {
    EnsureVisibleId(selected_id_);
  }
  if (notify) {
    CommitSelection();
  }
}

int TreeView::VisibleIndexOfId(int id) const {
  if (visible_dirty_) {
    const_cast<TreeView*>(this)->RebuildVisible();
  }
  for (int i = 0; i < static_cast<int>(visible_.size()); ++i) {
    if (visible_[static_cast<size_t>(i)].id == id) {
      return i;
    }
  }
  return -1;
}

void TreeView::EnsureVisibleId(int id) {
  if (visible_dirty_) {
    RebuildVisible();
  }
  int p = valid_id(id) ? nodes_[static_cast<size_t>(id)].parent : -1;
  bool changed = false;
  while (p >= 0) {
    if (!nodes_[static_cast<size_t>(p)].expanded) {
      nodes_[static_cast<size_t>(p)].expanded = true;
      changed = true;
    }
    p = nodes_[static_cast<size_t>(p)].parent;
  }
  if (changed) {
    visible_dirty_ = true;
    RebuildVisible();
  }
  const int row = VisibleIndexOfId(id);
  if (row < 0) {
    return;
  }
  const float top = static_cast<float>(row) * row_h_;
  const float bottom = top + row_h_;
  const float view_h = ViewportHeight();
  if (top < scroll_y_) {
    set_scroll_offset(top);
  } else if (bottom > scroll_y_ + view_h) {
    set_scroll_offset(bottom - view_h);
  }
}

float TreeView::ContentHeight() const {
  if (visible_dirty_) {
    const_cast<TreeView*>(this)->RebuildVisible();
  }
  return static_cast<float>(visible_.size()) * row_h_;
}

float TreeView::ViewportWidth() const {
  return NeedsScrollbar()
             ? std::max(0.f, bounds_.w - VerticalScrollbar::kWidth)
             : bounds_.w;
}

float TreeView::ViewportHeight() const { return bounds_.h; }

float TreeView::MaxScrollOffset() const {
  return std::max(0.f, ContentHeight() - ViewportHeight());
}

bool TreeView::NeedsScrollbar() const {
  const float vh = bounds_.h > 0.f
                       ? bounds_.h
                       : (preferred_height() > 0.f ? preferred_height() : 0.f);
  return ContentHeight() > vh && vh > 0.f;
}

void TreeView::ClampScroll() {
  scroll_y_ = std::clamp(scroll_y_, 0.f, MaxScrollOffset());
}

void TreeView::set_scroll_offset(float y) {
  scroll_y_ = y;
  ClampScroll();
}

void TreeView::SyncVScrollBar() {
  vscroll_.set_content_height(ContentHeight());
  vscroll_.set_viewport_height(ViewportHeight());
  vscroll_.set_scroll_offset(scroll_y_);
  vscroll_.set_track_bounds(
      RectF{bounds_.x + bounds_.w - VerticalScrollbar::kWidth, bounds_.y,
            VerticalScrollbar::kWidth, ViewportHeight()});
  scroll_y_ = vscroll_.scroll_offset();
}

void TreeView::VisibleRange(int* first, int* last) const {
  if (visible_dirty_) {
    const_cast<TreeView*>(this)->RebuildVisible();
  }
  const int n = static_cast<int>(visible_.size());
  if (n <= 0) {
    *first = 0;
    *last = -1;
    return;
  }
  *first = std::max(0, static_cast<int>(scroll_y_ / row_h_) - 2);
  *last = std::min(
      n - 1,
      static_cast<int>((scroll_y_ + ViewportHeight()) / row_h_) + 2);
}

RectF TreeView::RowRect(int row_index) const {
  const float top = static_cast<float>(row_index) * row_h_ - scroll_y_;
  return RectF{bounds_.x, bounds_.y + top, ViewportWidth(), row_h_};
}

RectF TreeView::TwistRect(const RectF& row, int depth) const {
  const float x = row.x + kPadX + static_cast<float>(depth) * indent_;
  const float s = kTwistSize;
  return RectF{x, row.y + (row.h - s) * 0.5f, s, s};
}

RectF TreeView::CheckRect(const RectF& row, int depth) const {
  const RectF twist = TwistRect(row, depth);
  const float x = twist.x + kTwistSize + 4.f;
  const float s = kCheckSize;
  return RectF{x, row.y + (row.h - s) * 0.5f, s, s};
}

bool TreeView::HitTwist(int row_index, float x, float y) const {
  if (row_index < 0 || row_index >= static_cast<int>(visible_.size())) {
    return false;
  }
  const VisibleRow& vr = visible_[static_cast<size_t>(row_index)];
  if (!CanExpand(vr.id)) {
    return false;
  }
  return ContainsPoint(TwistRect(RowRect(row_index), vr.depth), x, y);
}

bool TreeView::HitCheck(int row_index, float x, float y) const {
  if (!checkable_ || row_index < 0 ||
      row_index >= static_cast<int>(visible_.size())) {
    return false;
  }
  const VisibleRow& vr = visible_[static_cast<size_t>(row_index)];
  return ContainsPoint(CheckRect(RowRect(row_index), vr.depth), x, y);
}

int TreeView::RowAtPoint(float x, float y) const {
  const RectF vp{bounds_.x, bounds_.y, ViewportWidth(), ViewportHeight()};
  if (!ContainsPoint(vp, x, y)) {
    return -1;
  }
  if (visible_dirty_) {
    const_cast<TreeView*>(this)->RebuildVisible();
  }
  const int row = static_cast<int>(((y - bounds_.y) + scroll_y_) / row_h_);
  if (row < 0 || row >= static_cast<int>(visible_.size())) {
    return -1;
  }
  return row;
}

SizeF TreeView::Measure(float max_w, float max_h) {
  const float hug_h =
      preferred_height() > 0.f ? preferred_height() : 200.f;
  return ResolveSize(max_w, max_h,
                     preferred_width() > 0.f ? preferred_width() : max_w, hug_h);
}

void TreeView::Layout(const RectF& final_rect) {
  bounds_ = final_rect;
  if (visible_dirty_) {
    RebuildVisible();
  } else {
    ClampScroll();
  }
}

void TreeView::PaintCheckBox(auralite::Canvas& canvas, const RectF& box,
                             TreeCheckState state, bool selected) const {
  const ThemeTokens& th = Theme::Active();
  canvas.FillRect(box, th.surface);
  canvas.DrawRect(box, selected ? th.border_focus : th.border, 1.f);
  if (state == TreeCheckState::Checked) {
    canvas.DrawLine(box.x + 3.f, box.y + box.h * 0.55f, box.x + box.w * 0.4f,
                    box.y + box.h - 3.f, th.glyph, 1.8f);
    canvas.DrawLine(box.x + box.w * 0.4f, box.y + box.h - 3.f,
                    box.x + box.w - 3.f, box.y + 3.f, th.glyph, 1.8f);
  } else if (state == TreeCheckState::Partial) {
    canvas.FillRect(RectF{box.x + 3.f, box.y + box.h * 0.5f - 1.5f, box.w - 6.f, 3.f},
                    th.glyph);
  }
}

void TreeView::Paint(auralite::Canvas& canvas) {
  if (!visible()) {
    return;
  }
  if (visible_dirty_) {
    RebuildVisible();
  }

  const ThemeTokens& th = Theme::Active();
  const float fs = ResolveFontSize(font_size_);
  canvas.FillRect(bounds_, th.surface);
  canvas.DrawRect(bounds_, focused() ? th.border_focus : th.border,
                  focused() ? 1.5f : 1.f);

  const RectF clip{bounds_.x, bounds_.y, ViewportWidth(), ViewportHeight()};
  canvas.PushAxisAlignedClip(clip);

  int first = 0;
  int last = -1;
  VisibleRange(&first, &last);
  for (int i = first; i <= last; ++i) {
    const VisibleRow& vr = visible_[static_cast<size_t>(i)];
    const RectF row = RowRect(i);
    const bool selected = (vr.id == selected_id_);
    const bool hovered = (i == hover_row_);
    if (selected) {
      canvas.FillRect(row, th.accent);
    } else if (hovered) {
      canvas.FillRect(row, th.accent_soft);
    }

    const ColorF text_c = selected ? th.text_on_accent : th.text;
    const ColorF twist_c = selected ? th.text_on_accent : th.glyph;

    const RectF twist = TwistRect(row, vr.depth);
    if (CanExpand(vr.id)) {
      const float cx = twist.x + twist.w * 0.5f;
      const float cy = twist.y + twist.h * 0.5f;
      if (loading(vr.id)) {
        canvas.DrawRect(RectF{cx - 3.f, cy - 3.f, 6.f, 6.f}, twist_c, 1.f);
      } else if (expanded(vr.id)) {
        canvas.DrawLine(cx - 4.f, cy - 2.f, cx, cy + 3.f, twist_c, 1.5f);
        canvas.DrawLine(cx, cy + 3.f, cx + 4.f, cy - 2.f, twist_c, 1.5f);
      } else {
        canvas.DrawLine(cx - 2.f, cy - 4.f, cx + 3.f, cy, twist_c, 1.5f);
        canvas.DrawLine(cx + 3.f, cy, cx - 2.f, cy + 4.f, twist_c, 1.5f);
      }
    }

    float text_x = twist.x + kTwistSize + 4.f;
    if (checkable_) {
      const RectF box = CheckRect(row, vr.depth);
      PaintCheckBox(canvas, box, checked(vr.id), selected);
      text_x = box.x + box.w + 6.f;
    }

    std::wstring label = text(vr.id);
    if (loading(vr.id)) {
      label += L" …";
    }
    canvas.DrawText(label,
                    RectF{text_x, row.y, std::max(0.f, row.x + row.w - text_x - 4.f),
                          row.h},
                    text_c, fs, th.font_ui.c_str(),
                    auralite::TextHAlign::Left);
  }

  canvas.PopAxisAlignedClip();

  SyncVScrollBar();
  vscroll_.Paint(canvas);
}

void TreeView::OnMouseWheel(const MouseEvent& e) {
  if (e.wheel_delta == 0 || !NeedsScrollbar()) {
    return;
  }
  const float lines =
      static_cast<float>(e.wheel_delta) / static_cast<float>(WHEEL_DELTA);
  set_scroll_offset(scroll_y_ - lines * row_h_ * 3.f);
}

void TreeView::OnMouseDown(const MouseEvent& e) {
  if (e.button != MouseButton::Left) {
    return;
  }
  if (visible_dirty_) {
    RebuildVisible();
  }
  SyncVScrollBar();
  if (vscroll_.OnMouseDown(e)) {
    set_scroll_offset(vscroll_.scroll_offset());
    return;
  }

  const int row = RowAtPoint(e.x, e.y);
  if (row < 0) {
    return;
  }
  hover_row_ = row;
  if (HitTwist(row, e.x, e.y)) {
    const int id = visible_[static_cast<size_t>(row)].id;
    set_expanded(id, !expanded(id), true);
    return;
  }
  if (HitCheck(row, e.x, e.y)) {
    ToggleCheck(visible_[static_cast<size_t>(row)].id);
    return;
  }
  set_selected_id(visible_[static_cast<size_t>(row)].id, true);
}

void TreeView::OnMouseMove(const MouseEvent& e) {
  if (vscroll_.OnMouseMove(e)) {
    set_scroll_offset(vscroll_.scroll_offset());
    return;
  }
  hover_row_ = RowAtPoint(e.x, e.y);
}

void TreeView::OnMouseUp(const MouseEvent& e) { vscroll_.OnMouseUp(e); }

void TreeView::OnMouseLeave(const MouseEvent&) { hover_row_ = -1; }

void TreeView::OnKey(const KeyEvent& e) {
  if (!e.down) {
    return;
  }
  if (visible_dirty_) {
    RebuildVisible();
  }
  if (visible_.empty()) {
    return;
  }

  int row = VisibleIndexOfId(selected_id_);
  if (e.vk == VK_LEFT) {
    if (selected_id_ >= 0 && CanExpand(selected_id_) &&
        expanded(selected_id_)) {
      set_expanded(selected_id_, false, true);
    } else if (selected_id_ >= 0) {
      const int p = nodes_[static_cast<size_t>(selected_id_)].parent;
      if (p >= 0) {
        set_selected_id(p, true);
      }
    }
    return;
  }
  if (e.vk == VK_RIGHT) {
    if (selected_id_ >= 0 && CanExpand(selected_id_)) {
      if (!expanded(selected_id_)) {
        set_expanded(selected_id_, true, true);
      } else {
        const auto& ch = nodes_[static_cast<size_t>(selected_id_)].children;
        if (!ch.empty()) {
          set_selected_id(ch.front(), true);
        }
      }
    }
    return;
  }
  if (e.vk == VK_SPACE) {
    if (checkable_ && selected_id_ >= 0) {
      ToggleCheck(selected_id_);
    } else if (selected_id_ >= 0 && CanExpand(selected_id_)) {
      set_expanded(selected_id_, !expanded(selected_id_), true);
    } else {
      CommitSelection();
    }
    return;
  }
  if (e.vk == VK_RETURN) {
    if (selected_id_ >= 0 && CanExpand(selected_id_)) {
      set_expanded(selected_id_, !expanded(selected_id_), true);
    } else {
      CommitSelection();
    }
    return;
  }
  if (e.vk == VK_UP) {
    if (row < 0) {
      set_selected_id(visible_.front().id, true);
    } else if (row > 0) {
      set_selected_id(visible_[static_cast<size_t>(row - 1)].id, true);
    }
  } else if (e.vk == VK_DOWN) {
    if (row < 0) {
      set_selected_id(visible_.front().id, true);
    } else if (row + 1 < static_cast<int>(visible_.size())) {
      set_selected_id(visible_[static_cast<size_t>(row + 1)].id, true);
    }
  } else if (e.vk == VK_HOME) {
    set_selected_id(visible_.front().id, true);
  } else if (e.vk == VK_END) {
    set_selected_id(visible_.back().id, true);
  }
}

}  // namespace auralite::ui
