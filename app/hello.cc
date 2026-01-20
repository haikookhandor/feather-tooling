#include <fmt/color.h>
#include <fmt/core.h>

int main() {
  fmt::print(fmt::fg(fmt::color::light_sea_green),
             "Hello from {} (C++{})\n", "FeatherTooling", __cplusplus);
  return 0;
}
