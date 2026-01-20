#pragma once
#include "absl/status/statusor.h"

namespace common {

// Returns a+b if both are non-negative; otherwise InvalidArgument.
absl::StatusOr<int> AddNonNegative(int a, int b);

}  // namespace common
