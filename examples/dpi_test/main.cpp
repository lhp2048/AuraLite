#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "auralite/canvas.h"

namespace {

int g_failures = 0;

void Expect(bool cond, const char* name) {
  if (!cond) {
    std::printf("FAIL %s\n", name);
    ++g_failures;
  } else {
    std::printf("ok   %s\n", name);
  }
}

void ExpectNear(float got, float want, const char* name, float eps = 0.01f) {
  if (std::fabs(got - want) > eps) {
    std::printf("FAIL %s: got=%.4f want=%.4f\n", name, got, want);
    ++g_failures;
  } else {
    std::printf("ok   %s\n", name);
  }
}

}  // namespace

int main() {
  using auralite::DipFromPx;
  using auralite::PxFromDip;
  const float dpis[] = {96.f, 120.f, 144.f, 192.f};
  for (float dpi : dpis) {
    ExpectNear(PxFromDip(96.f, dpi), dpi, "96dip -> dpi px");
    ExpectNear(DipFromPx(dpi, dpi), 96.f, "dpi px -> 96dip");
    ExpectNear(DipFromPx(PxFromDip(800.f, dpi), dpi), 800.f, "roundtrip 800");
  }
  ExpectNear(PxFromDip(96.f, 0.f), 96.f, "dpi 0 treated as 96");
  ExpectNear(PxFromDip(96.f, -1.f), 96.f, "dpi neg treated as 96");
  Expect(std::ceil(PxFromDip(800.f, 144.f)) == 1200.0, "ceil 800dip @144 = 1200");
  if (g_failures) {
    std::printf("%d failed\n", g_failures);
    return 1;
  }
  std::printf("all passed\n");
  return 0;
}
