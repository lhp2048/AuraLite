#include "mx/reactive/signal.h"
#include "mx/reactive/observe.h"

#include <cstdio>
#include <string>

int main() {
  using namespace mx::reactive;
  int hits = 0;
  Signal<int> n{0};
  auto sub = Observe([&] {
    (void)n.Get();
    ++hits;
  });
  // Observe runs once at subscribe
  if (hits != 1) return 1;
  n.Set(1);
  if (hits != 2) return 2;
  n.Set(1);  // equal skip
  if (hits != 2) return 3;
  {
    Signal<int> a{1}, b{2};
    Computed<int> sum{[&] { return a.Get() + b.Get(); }};
    int seen = -1;
    auto s2 = Observe([&] { seen = sum.Get(); });
    if (seen != 3) return 4;
    a.Set(5);
    if (seen != 7) return 5;
  }
  sub = {};  // unsubscribe
  n.Set(9);
  if (hits != 2) return 6;

  // Batch: two Sets → one Observe flush
  Signal<int> x{0};
  Signal<int> y{0};
  int batch_hits = 0;
  auto batch_sub = Observe([&] {
    (void)x.Get();
    (void)y.Get();
    ++batch_hits;
  });
  if (batch_hits != 1) return 7;
  Batch([&] {
    x.Set(1);
    y.Set(2);
  });
  if (batch_hits != 2) return 8;
  x.Set(3);
  if (batch_hits != 3) return 9;
  (void)batch_sub;

  std::puts("reactive_test ok");
  return 0;
}
