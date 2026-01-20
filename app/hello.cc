#include <fmt/color.h>
#include <fmt/core.h>
#include <spdlog/spdlog.h>

#include "absl/time/clock.h"
#include "absl/time/time.h"
#include "common/logging.h"
#include "common/status_demo.h"

int main() {
  // 1) Rotating logs
  common::InitLogging("logs");  // creates logs/service.log (10MB, 3 files)
  spdlog::info("App starting up...");

  // Keep your colored hello
  fmt::print(fmt::fg(fmt::color::light_sea_green),
             "Hello from {} (C++{})\n", "FeatherTooling", __cplusplus);

  // 2) StatusOr example
  auto ok = common::AddNonNegative(2, 3);
  if (!ok.ok()) {
    spdlog::error("AddNonNegative failed: {}", ok.status().message());
  } else {
    spdlog::info("AddNonNegative(2,3) = {}", *ok);
  }

  auto bad = common::AddNonNegative(-1, 5);
  if (!bad.ok()) {
    spdlog::warn("Expected failure: {}", bad.status().message());
  }

  // 3) Time a dummy operation with Abseil Time
  const auto t0 = absl::Now();
  volatile double s = 0.0;
  for (int i = 0; i < 5'000'000; ++i) s += i * 0.000001;
  const auto t1 = absl::Now();
  absl::Duration dt = t1 - t0;

  spdlog::info("Dummy op elapsed: {:.3f} ms", absl::ToDoubleMilliseconds(dt));
  spdlog::info("App done.");
  return 0;
}
