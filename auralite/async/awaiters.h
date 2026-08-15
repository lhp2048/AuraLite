#pragma once

#include "auralite/async/task_lambda.h"

#include <atomic>
#include <coroutine>
#include <functional>
#include <memory>
#include <utility>

namespace auralite::async {

// Optional gate from Window::alive_flag(). When false, posted resumes destroy
// the coroutine frame on the UI thread instead of continuing.
using AliveFlag = std::shared_ptr<std::atomic_bool>;

void LogUnhandledException();

// Fire-and-forget UI coroutine. Starts immediately (no initial suspend);
// the frame is destroyed on final_suspend. Exceptions are logged, not
// std::terminate.
//
// MSVC: do not start coroutines from a temporary lambda
// (SpawnUi([...]() -> FireAndForget { ... })). Prefer a free function or a
// named callable whose lifetime outlives the first suspend.
struct FireAndForget {
  struct promise_type {
    FireAndForget get_return_object() noexcept { return {}; }
    std::suspend_never initial_suspend() noexcept { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() noexcept {}
    void unhandled_exception() { LogUnhandledException(); }
  };
};

void SpawnUi(FireAndForget) noexcept;
void SpawnUi(std::coroutine_handle<> handle);

// Call once MessageLoopForUI exists (e.g. start of Application::Run).
void EnsureUiProxy();

struct ResumeOnUiAwaiter {
  AliveFlag alive;
  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<> h) const;
  void await_resume() const noexcept {}
};

inline ResumeOnUiAwaiter ResumeOnUi(AliveFlag alive = {}) {
  return ResumeOnUiAwaiter{std::move(alive)};
}

struct DelayAwaiter {
  int ms = 0;
  AliveFlag alive;
  bool await_ready() const noexcept { return ms <= 0; }
  void await_suspend(std::coroutine_handle<> h) const;
  void await_resume() const noexcept {}
};

inline DelayAwaiter Delay(int ms, AliveFlag alive = {}) {
  return DelayAwaiter{ms, std::move(alive)};
}

struct RunAsyncAwaiter {
  std::function<void()> fn;
  AliveFlag alive;
  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<> h);
  void await_resume() const noexcept {}
};

inline RunAsyncAwaiter RunAsync(std::function<void()> fn, AliveFlag alive = {}) {
  return RunAsyncAwaiter{std::move(fn), std::move(alive)};
}

}  // namespace auralite::async
