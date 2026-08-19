#pragma once

#include <cstdint>
#include <vector>

namespace mx::reactive::detail {

struct ISignal {
  virtual ~ISignal() = default;
  virtual void Subscribe(class Observer*) = 0;
  virtual void Unsubscribe(class Observer*) = 0;
};

class Observer {
 public:
  virtual ~Observer();
  virtual void OnDepsDirty() = 0;
  void Track(ISignal* s);
  void ClearDeps();
  // Drop a dying signal without calling Unsubscribe (signal is already gone).
  void DetachSignal(ISignal* s);

 private:
  std::vector<ISignal*> deps_;
};

Observer*& CurrentObserver();  // thread_local
std::uint32_t& BatchDepth();
void FlushPending();
void QueueObserver(Observer* o);

// Debug: first caller pins the UI thread; later calls must match.
void AssertUiThread();

}  // namespace mx::reactive::detail
