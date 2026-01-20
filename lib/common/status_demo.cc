#include "common/status_demo.h"
#include "absl/status/status.h"

namespace common {

absl::StatusOr<int> AddNonNegative(int a, int b) {
  if (a < 0 || b < 0) {
    return absl::InvalidArgumentError("inputs must be non-negative");
  }
  return a + b;
}

}  // namespace common
