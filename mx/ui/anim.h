#pragma once

#include <cstdint>
#include <functional>
#include <vector>

namespace mx::ui {

class Window;

enum class Easing { Linear, EaseOutCubic };

constexpr float kUiAnimSec = 0.15f;

class AnimationDriver {
 public:
  using TickFn = std::function<void(float t)>;
  using DoneFn = std::function<void()>;

  // |now_sec| is an arbitrary monotonic clock. duration<=0 runs tick(1)+done
  // immediately and returns 0.
  uint64_t Start(float duration_sec, Easing easing, TickFn tick, DoneFn done,
                 double now_sec);
  void Cancel(uint64_t id);
  // Advance running items. Returns true if any remain.
  bool Tick(double now_sec);
  bool empty() const { return items_.empty(); }
  void Clear();

 private:
  struct Item {
    uint64_t id = 0;
    double start_sec = 0.0;
    float duration_sec = 0.f;
    Easing easing = Easing::Linear;
    TickFn tick;
    DoneFn done;
  };

  static float ApplyEasing(Easing easing, float t);

  std::vector<Item> items_;
  uint64_t next_id_ = 1;
};

// Owns one Window::Animate id. Cancel is skipped if the HWND is already gone.
class Tween {
 public:
  Tween() = default;
  ~Tween() { Cancel(); }
  Tween(const Tween&) = delete;
  Tween& operator=(const Tween&) = delete;

  void Start(Window* window, float duration_sec, Easing easing,
             AnimationDriver::TickFn tick, AnimationDriver::DoneFn done = {});
  void Cancel();
  bool running() const { return id_ != 0; }

 private:
  Window* window_ = nullptr;
  uint64_t id_ = 0;
};

}  // namespace mx::ui
