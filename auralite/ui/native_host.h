#pragma once

#include "auralite/ui/node.h"

#include <windows.h>

namespace auralite::ui {

// Who destroys the guest HWND.
enum class NativeLifetime {
  Owned,     // NativeHost / host Window teardown DestroyWindow (typical)
  Borrowed,  // External HWND; never destroyed by NativeHost
};

// Black-box HWND hole. Syncs DIP bounds and visibility. Does not paint,
// forward input, or clip for ScrollView.
// Who created the HWND does not matter; |NativeLifetime| only chooses
// whether this node DestroyWindow's it.
class NativeHost : public Node {
 public:
  NativeHost();
  ~NativeHost() override;

  NativeHost(const NativeHost&) = delete;
  NativeHost& operator=(const NativeHost&) = delete;

  // Takes ownership: Detach / host teardown DestroyWindow.
  // Use this for both in-control CreateWindow and handed-in HWNDs you want
  // torn down with the UI.
  NativeHost& Attach(HWND hwnd);
  NativeHost& Attach(HWND hwnd, NativeLifetime life);
  // Same as Attach(hwnd, NativeLifetime::Borrowed): keep the HWND alive.
  NativeHost& AttachBorrowed(HWND hwnd);

  // Owned: DestroyWindow. Borrowed: unparent and restore style.
  void Detach();
  // Unparent without destroying; caller owns the HWND afterwards.
  HWND Release();

  HWND hwnd() const { return hwnd_; }
  bool owns_hwnd() const { return attached_ && owned_; }

  SizeF Measure(float max_w, float max_h) override;
  void Layout(const RectF& final_rect) override;

  // Host HWND is going away: destroy owned guests, unparent borrowed ones.
  static void OrphanTree(Node* root);
  // After parent DIB present: redraw guests whose rect intersects |present_px|.
  static void RedrawGuests(HWND parent, const RECT& present_px);

 protected:
  void OnHostWindowChanged() override;

 private:
  bool ShownInTree() const;
  void AdoptParent();
  void HideGuest();
  void InvalidateSyncedRect();
  void SyncNative();
  void Drop(bool destroy_owned);
  static void RegisterLive(NativeHost* host);
  static void UnregisterLive(NativeHost* host);

  HWND hwnd_ = nullptr;
  HWND orig_parent_ = nullptr;
  LONG orig_style_ = 0;
  bool attached_ = false;
  bool owned_ = true;
  int synced_x_ = 0;
  int synced_y_ = 0;
  int synced_w_ = 0;
  int synced_h_ = 0;
  bool synced_visible_ = false;
};

}  // namespace auralite::ui
