#include "auralite/async/awaiters.h"

#include "base/at_exit.h"
#include "message_framework/message_loop.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <memory>
#include <thread>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace {

bool g_done = false;
int g_worker_ran = 0;
int g_posted = 0;
int g_cancelled_resumed = 0;
DWORD g_ui_tid = 0;
DWORD g_after_async_tid = 0;

auralite::async::FireAndForget RunCancelled(
    std::shared_ptr<std::atomic_bool> alive) {
  using namespace auralite::async;
  co_await Delay(5, alive);
  ++g_cancelled_resumed;
  MessageLoop::current()->Quit();
}

auralite::async::FireAndForget RunScenario() {
  using namespace auralite::async;

  PostFn([] { g_posted = 1; });
  co_await ResumeOnUi();
  if (g_posted != 1) {
    MessageLoop::current()->Quit();
    co_return;
  }

  auto dead = std::make_shared<std::atomic_bool>(false);
  SpawnUi(RunCancelled(dead));
  co_await Delay(25);
  if (g_cancelled_resumed != 0) {
    MessageLoop::current()->Quit();
    co_return;
  }

  co_await Delay(10);
  co_await RunAsync([] {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    g_worker_ran = 1;
  });

  g_after_async_tid = GetCurrentThreadId();
  g_done = true;
  MessageLoop::current()->Quit();
}

}  // namespace

int main() {
  base::AtExitManager exit_manager;
  MessageLoopForUI loop;
  g_ui_tid = GetCurrentThreadId();
  auralite::async::SpawnUi(RunScenario());
  loop.Run(nullptr);
  if (g_posted != 1) {
    std::puts("async_test FAIL: PostFn / ResumeOnUi");
    return 1;
  }
  if (g_cancelled_resumed != 0) {
    std::puts("async_test FAIL: dead AliveFlag still resumed");
    return 5;
  }
  if (g_worker_ran != 1) {
    std::puts("async_test FAIL: RunAsync worker");
    return 2;
  }
  if (!g_done) {
    std::puts("async_test FAIL: scenario did not finish");
    return 3;
  }
  if (g_after_async_tid != g_ui_tid) {
    std::puts("async_test FAIL: did not resume on UI thread");
    return 4;
  }
  std::puts("async_test ok");
  return 0;
}
