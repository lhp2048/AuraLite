#pragma once

#include <functional>
#include <memory>

namespace mx::reactive {

class Subscription {
 public:
  Subscription() noexcept;
  ~Subscription();
  Subscription(Subscription&& other) noexcept;
  Subscription& operator=(Subscription&& other) noexcept;

  Subscription(const Subscription&) = delete;
  Subscription& operator=(const Subscription&) = delete;

 private:
  friend Subscription Observe(std::function<void()> effect);
  struct Impl;
  explicit Subscription(std::unique_ptr<Impl> impl) noexcept;
  std::unique_ptr<Impl> impl_;
};

Subscription Observe(std::function<void()> effect);

// Coalesce Signal notifications: observers run once when |fn| returns.
void Batch(std::function<void()> fn);

}  // namespace mx::reactive
