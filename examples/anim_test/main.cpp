#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "auralite/ui/anim.h"

namespace {

int g_failures = 0;

void Expect(const char* name, bool cond) {
  if (!cond) {
    std::printf("FAIL %s\n", name);
    ++g_failures;
  } else {
    std::printf("ok   %s\n", name);
  }
}

void ExpectNear(const char* name, float got, float want, float eps = 0.001f) {
  if (std::fabs(got - want) > eps) {
    std::printf("FAIL %s: got=%.4f want=%.4f\n", name, got, want);
    ++g_failures;
  } else {
    std::printf("ok   %s\n", name);
  }
}

}  // namespace

int main() {
  setvbuf(stdout, nullptr, _IONBF, 0);
  using auralite::ui::AnimationDriver;
  using auralite::ui::Easing;

  {
    AnimationDriver d;
    float last = -1.f;
    int done = 0;
    const uint64_t id =
        d.Start(1.f, Easing::Linear, [&](float t) { last = t; },
                [&] { ++done; }, 0.0);
    Expect("id nonzero", id != 0);
    Expect("not empty", !d.empty());
    d.Tick(0.0);
    ExpectNear("t0", last, 0.f);
    d.Tick(0.5);
    ExpectNear("t mid", last, 0.5f);
    Expect("not done yet", done == 0);
    Expect("completes at end", !d.Tick(1.0));
    ExpectNear("t1", last, 1.f);
    Expect("done once", done == 1);
    Expect("empty after end", d.empty());
    Expect("tick empty false", !d.Tick(2.0));
  }

  {
    AnimationDriver d;
    float last = -1.f;
    d.Start(1.f, Easing::EaseOutCubic, [&](float t) { last = t; }, {}, 0.0);
    d.Tick(0.5);
    // 1 - (1-0.5)^3 = 0.875
    ExpectNear("ease out cubic mid", last, 0.875f);
  }

  {
    AnimationDriver d;
    int ticks = 0;
    int done = 0;
    Expect("zero duration id 0",
           d.Start(0.f, Easing::Linear, [&](float) { ++ticks; },
                   [&] { ++done; }, 0.0) == 0);
    Expect("zero duration tick+done", ticks == 1 && done == 1);
    Expect("zero duration empty", d.empty());
  }

  {
    AnimationDriver d;
    int done = 0;
    float last = -1.f;
    const uint64_t id =
        d.Start(1.f, Easing::Linear, [&](float t) { last = t; },
                [&] { ++done; }, 10.0);
    d.Tick(10.25);
    ExpectNear("t 0.25", last, 0.25f);
    d.Cancel(id);
    Expect("cancel empties", d.empty());
    d.Tick(11.0);
    Expect("cancel skips done", done == 0);
    d.Cancel(id);
    Expect("cancel missing ok", d.empty());
  }

  {
    AnimationDriver d;
    int done = 0;
    d.Start(1.f, Easing::Linear, {}, [&] { ++done; }, 0.0);
    d.Clear();
    Expect("clear empties", d.empty());
    d.Tick(1.0);
    Expect("clear skips done", done == 0);
  }

  if (g_failures) {
    std::printf("%d failed\n", g_failures);
    return EXIT_FAILURE;
  }
  std::printf("all ok\n");
  return EXIT_SUCCESS;
}
