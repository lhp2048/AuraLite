#pragma once

#include "mx/reactive/detail/tracker.h"

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

namespace mx::reactive {
namespace detail {

class SlotList : public ISignal {
 public:
  SlotList() = default;
  SlotList(const SlotList&) = delete;
  SlotList& operator=(const SlotList&) = delete;
  SlotList(SlotList&&) = delete;
  SlotList& operator=(SlotList&&) = delete;

  ~SlotList() override {
    auto copy = observers_;
    observers_.clear();
    for (Observer* o : copy) {
      if (o) {
        o->DetachSignal(this);
      }
    }
  }

  void Subscribe(Observer* o) override {
    if (!o) {
      return;
    }
    if (std::find(observers_.begin(), observers_.end(), o) != observers_.end()) {
      return;
    }
    observers_.push_back(o);
  }

  void Unsubscribe(Observer* o) override {
    observers_.erase(std::remove(observers_.begin(), observers_.end(), o),
                     observers_.end());
  }

 protected:
  void Notify() {
    auto copy = observers_;
    for (Observer* o : copy) {
      QueueObserver(o);
    }
    if (BatchDepth() == 0) {
      FlushPending();
    }
  }

 private:
  std::vector<Observer*> observers_;
};

}  // namespace detail

template <typename T>
class Signal : public detail::SlotList {
 public:
  explicit Signal(T init = T{}) : value_(std::move(init)) {}

  const T& Peek() const {
    detail::AssertUiThread();
    return value_;
  }

  T Get() const {
    detail::AssertUiThread();
    if (auto* o = detail::CurrentObserver()) {
      o->Track(const_cast<Signal*>(this));
    }
    return value_;
  }

  void Set(T next) {
    detail::AssertUiThread();
    if constexpr (requires { value_ == next; }) {
      if (value_ == next) {
        return;
      }
    }
    value_ = std::move(next);
    this->Notify();
  }

 private:
  T value_;
};

// Whole-list replace: Set always notifies (skip operator==).
template <typename U>
class Signal<std::vector<U>> : public detail::SlotList {
 public:
  explicit Signal(std::vector<U> init = {}) : value_(std::move(init)) {}

  const std::vector<U>& Peek() const {
    detail::AssertUiThread();
    return value_;
  }

  const std::vector<U>& Get() const {
    detail::AssertUiThread();
    if (auto* o = detail::CurrentObserver()) {
      o->Track(const_cast<Signal*>(this));
    }
    return value_;
  }

  void Set(std::vector<U> next) {
    detail::AssertUiThread();
    value_ = std::move(next);
    this->Notify();
  }

 private:
  std::vector<U> value_;
};

template <typename T>
class Computed {
 public:
  explicit Computed(std::function<T()> fn) : fn_(std::move(fn)) {}

  T Get() const {
    detail::AssertUiThread();
    // v1: recompute every Get(); nested Signal::Get tracks the active Observe.
    return fn_();
  }

 private:
  std::function<T()> fn_;
};

}  // namespace mx::reactive
