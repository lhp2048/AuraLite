#include "mx/reactive/observe.h"
#include "mx/reactive/detail/tracker.h"

#include <algorithm>
#include <cassert>
#include <utility>
#include <vector>


#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace mx::reactive {
namespace detail {
namespace {

thread_local Observer* g_current = nullptr;
thread_local std::uint32_t g_batch_depth = 0;
thread_local std::vector<Observer*> g_pending;
thread_local bool g_flushing = false;

#ifndef NDEBUG
DWORD g_ui_thread_id = 0;
#endif

void UnqueueObserver(Observer* o) {
  g_pending.erase(std::remove(g_pending.begin(), g_pending.end(), o),
                  g_pending.end());
}

}  // namespace

Observer*& CurrentObserver() {
  return g_current;
}

std::uint32_t& BatchDepth() {
  return g_batch_depth;
}

void AssertUiThread() {
#ifndef NDEBUG
  const DWORD tid = GetCurrentThreadId();
  if (g_ui_thread_id == 0) {
    g_ui_thread_id = tid;
    return;
  }
  assert(tid == g_ui_thread_id &&
         "mx::reactive: Signal/Computed/Observe only on UI thread");
#else
  (void)0;
#endif
}

void QueueObserver(Observer* o) {
  if (!o) {
    return;
  }
  if (std::find(g_pending.begin(), g_pending.end(), o) != g_pending.end()) {
    return;
  }
  g_pending.push_back(o);
}

void FlushPending() {
  if (g_flushing) {
    return;
  }
  g_flushing = true;
  struct FlushGuard {
    bool& flag;
    ~FlushGuard() { flag = false; }
  } guard{g_flushing};
  while (!g_pending.empty()) {
    Observer* o = g_pending.front();
    g_pending.erase(g_pending.begin());
    o->OnDepsDirty();
  }
}

Observer::~Observer() {
  UnqueueObserver(this);
  if (g_current == this) {
    g_current = nullptr;
  }
  ClearDeps();
}

void Observer::Track(ISignal* s) {
  if (!s) {
    return;
  }
  if (std::find(deps_.begin(), deps_.end(), s) != deps_.end()) {
    return;
  }
  deps_.push_back(s);
  s->Subscribe(this);
}

void Observer::ClearDeps() {
  for (ISignal* s : deps_) {
    if (s) {
      s->Unsubscribe(this);
    }
  }
  deps_.clear();
}

void Observer::DetachSignal(ISignal* s) {
  deps_.erase(std::remove(deps_.begin(), deps_.end(), s), deps_.end());
}

}  // namespace detail

namespace {

class Effect final : public detail::Observer {
 public:
  explicit Effect(std::function<void()> fn) : fn_(std::move(fn)) {}

  void Run() {
    if (disposed_ || !fn_) {
      return;
    }
    ClearDeps();
    detail::Observer*& current = detail::CurrentObserver();
    struct ObserverScope {
      detail::Observer*& slot;
      detail::Observer* prev;
      ~ObserverScope() { slot = prev; }
    } scope{current, current};
    current = this;
    fn_();
  }

  void OnDepsDirty() override { Run(); }

  void Dispose() {
    disposed_ = true;
    ClearDeps();
  }

 private:
  std::function<void()> fn_;
  bool disposed_ = false;
};

}  // namespace

struct Subscription::Impl {
  explicit Impl(std::function<void()> fn) : effect(std::move(fn)) {}
  Effect effect;
};

Subscription::Subscription() noexcept = default;

Subscription::Subscription(std::unique_ptr<Impl> impl) noexcept
    : impl_(std::move(impl)) {}

Subscription::~Subscription() {
  if (impl_) {
    impl_->effect.Dispose();
  }
}

Subscription::Subscription(Subscription&& other) noexcept
    : impl_(std::move(other.impl_)) {}

Subscription& Subscription::operator=(Subscription&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  if (impl_) {
    impl_->effect.Dispose();
  }
  impl_ = std::move(other.impl_);
  return *this;
}

Subscription Observe(std::function<void()> effect) {
  detail::AssertUiThread();
  auto impl = std::make_unique<Subscription::Impl>(std::move(effect));
  impl->effect.Run();
  return Subscription(std::move(impl));
}

void Batch(std::function<void()> fn) {
  detail::AssertUiThread();
  ++detail::BatchDepth();
  struct Guard {
    ~Guard() {
      --detail::BatchDepth();
      if (detail::BatchDepth() == 0) {
        detail::FlushPending();
      }
    }
  } guard;
  if (fn) {
    fn();
  }
}

}  // namespace mx::reactive
