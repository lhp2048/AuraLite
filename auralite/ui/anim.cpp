#include "auralite/ui/anim.h"

#include "auralite/ui/window.h"

#include <algorithm>

namespace auralite::ui {

float AnimationDriver::ApplyEasing(Easing easing, float t) {
  t = std::clamp(t, 0.f, 1.f);
  if (easing == Easing::EaseOutCubic) {
    const float u = 1.f - t;
    return 1.f - u * u * u;
  }
  return t;
}

uint64_t AnimationDriver::Start(float duration_sec, Easing easing, TickFn tick,
                                DoneFn done, double now_sec) {
  if (duration_sec <= 0.f) {
    if (tick) {
      tick(1.f);
    }
    if (done) {
      done();
    }
    return 0;
  }
  Item item;
  item.id = next_id_++;
  item.start_sec = now_sec;
  item.duration_sec = duration_sec;
  item.easing = easing;
  item.tick = std::move(tick);
  item.done = std::move(done);
  const uint64_t id = item.id;
  items_.push_back(std::move(item));
  return id;
}

void AnimationDriver::Cancel(uint64_t id) {
  if (id == 0) {
    return;
  }
  items_.erase(std::remove_if(items_.begin(), items_.end(),
                              [id](const Item& it) { return it.id == id; }),
               items_.end());
}

void AnimationDriver::Clear() { items_.clear(); }

bool AnimationDriver::Tick(double now_sec) {
  std::vector<uint64_t> ids;
  ids.reserve(items_.size());
  for (const Item& it : items_) {
    ids.push_back(it.id);
  }
  for (uint64_t id : ids) {
    auto it = std::find_if(items_.begin(), items_.end(),
                           [id](const Item& item) { return item.id == id; });
    if (it == items_.end()) {
      continue;
    }
    const float dur = it->duration_sec;
    float u = dur <= 0.f ? 1.f
                         : static_cast<float>((now_sec - it->start_sec) / dur);
    if (u >= 1.f) {
      TickFn tick = std::move(it->tick);
      DoneFn done = std::move(it->done);
      items_.erase(it);
      if (tick) {
        tick(1.f);
      }
      if (done) {
        done();
      }
    } else {
      if (it->tick) {
        it->tick(ApplyEasing(it->easing, std::max(0.f, u)));
      }
    }
  }
  return !items_.empty();
}

void Tween::Start(Window* window, float duration_sec, Easing easing,
                  AnimationDriver::TickFn tick, AnimationDriver::DoneFn done) {
  Cancel();
  if (!window || !window->hwnd() || duration_sec <= 0.f) {
    if (tick) {
      tick(1.f);
    }
    if (done) {
      done();
    }
    return;
  }
  window_ = window;
  id_ = window->Animate(
      duration_sec, easing, std::move(tick),
      [this, done = std::move(done)]() {
        id_ = 0;
        window_ = nullptr;
        if (done) {
          done();
        }
      });
}

void Tween::Cancel() {
  if (id_ && window_ && window_->hwnd()) {
    window_->CancelAnimation(id_);
  }
  id_ = 0;
  window_ = nullptr;
}

}  // namespace auralite::ui
