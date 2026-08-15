#pragma once

#include "message_framework/message_loop.h"
#include "message_framework/task.h"

#include <functional>
#include <utility>

namespace auralite::async {

class LambdaTask : public Task {
 public:
  explicit LambdaTask(std::function<void()> fn) : fn_(std::move(fn)) {}
  void Run() override {
    if (fn_) {
      fn_();
    }
  }

 private:
  std::function<void()> fn_;
};

inline void PostTo(MessageLoop* loop, std::function<void()> fn) {
  if (!loop) {
    return;
  }
  loop->PostTask(new LambdaTask(std::move(fn)));
}

inline void PostFn(std::function<void()> fn) {
  PostTo(MessageLoop::current(), std::move(fn));
}

}  // namespace auralite::async
