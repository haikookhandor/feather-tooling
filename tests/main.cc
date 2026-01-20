#include <cassert>
#include <fmt/core.h>

int add(int a, int b) { return a + b; }

int main() {
  fmt::print("Running tiny tests...\n");
  assert(add(2, 3) == 5);
  assert(add(-1, 1) == 0);
  fmt::print("All tests passed.\n");
  return 0;
}
