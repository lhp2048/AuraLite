#include "auralite/async/awaiters.h"

#include "message_framework/message_loop_proxy.h"

#include <cstdio>
#include <exception>
#include <thread>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace auralite::async {
namespace {

scoped_refptr<base::MessageLoopProxy> g_ui_proxy;

void RememberUiProxy() {
  if (MessageLoop::current()) {
    g_ui_proxy = base::MessageLoopProxy::CreateForCurrentThread();
  }
}

scoped_refptr<base::MessageLoopProxy> UiProxy() {
  if (MessageLoop::current()) {
    RememberUiProxy();
  }
  return g_ui_proxy;
}

bool IsAlive(const AliveFlag& alive) {
  return !alive || alive->load(std::memory_order_acquire);
}

void ResumeOrDestroy(const scoped_refptr<base::MessageLoopProxy>& proxy,
                     std::coroutine_handle<> h,
                     AliveFlag alive) {
  if (!h) {
    return;
  }
  auto continue_on_ui = [h, alive = std::move(alive)]() mutable {
    if (!IsAlive(alive)) {
      h.destroy();
      return;
    }
    h.resume();
  };
  if (!proxy) {
    // Never destroy a UI coroutine frame on a worker thread.
    if (MessageLoop::current()) {
      continue_on_ui();
    }
    return;
  }
  const bool posted =
      proxy->PostTask(new LambdaTask(std::move(continue_on_ui)));
  if (!posted && MessageLoop::current()) {
    // Loop rejecting posts while still current — drop safely on UI.
    h.destroy();
  }
  // If the loop is gone, leak rather than destroy off-thread.
}

void EnsureUiProxyFromCurrentLoop() {
  RememberUiProxy();
}

}  // namespace

void EnsureUiProxy() {
  EnsureUiProxyFromCurrentLoop();
}

void LogUnhandledException() {
  try {
    if (std::current_exception()) {
      std::rethrow_exception(std::current_exception());
    }
  } catch (const std::exception& e) {
    char buf[512];
    std::snprintf(buf, sizeof(buf), "auralite::async unhandled: %s\n", e.what());
    std::fputs(buf, stderr);
    OutputDebugStringA(buf);
  } catch (...) {
    const char* msg = "auralite::async unhandled: unknown exception\n";
    std::fputs(msg, stderr);
    OutputDebugStringA(msg);
  }
}

void SpawnUi(FireAndForget) noexcept {
  RememberUiProxy();
}

void SpawnUi(std::coroutine_handle<> handle) {
  RememberUiProxy();
  ResumeOrDestroy(UiProxy(), handle, {});
}

void ResumeOnUiAwaiter::await_suspend(std::coroutine_handle<> h) const {
  ResumeOrDestroy(UiProxy(), h, alive);
}

void DelayAwaiter::await_suspend(std::coroutine_handle<> h) const {
  RememberUiProxy();
  auto proxy = UiProxy();
  AliveFlag gate = alive;
  if (!proxy) {
    if (h && MessageLoop::current()) {
      h.destroy();
    }
    return;
  }
  const bool posted = proxy->PostDelayedTask(
      new LambdaTask([h, gate = std::move(gate)]() mutable {
        if (!IsAlive(gate)) {
          h.destroy();
          return;
        }
        h.resume();
      }),
      static_cast<int64>(ms));
  if (!posted && h && MessageLoop::current()) {
    h.destroy();
  }
}

void RunAsyncAwaiter::await_suspend(std::coroutine_handle<> h) {
  RememberUiProxy();
  auto proxy = UiProxy();
  std::function<void()> work = std::move(fn);
  AliveFlag gate = std::move(alive);
  std::thread([work = std::move(work), proxy, h, gate = std::move(gate)]() mutable {
    try {
      if (work) {
        work();
      }
    } catch (...) {
      LogUnhandledException();
    }
    ResumeOrDestroy(proxy, h, std::move(gate));
  }).detach();
}

}  // namespace auralite::async
