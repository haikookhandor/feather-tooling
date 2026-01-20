add_executable(hello hello.cc)
target_link_libraries(hello
  PRIVATE
    fmt::fmt
    spdlog::spdlog
    absl::status
    absl::statusor
    absl::time
    common          # our new library
)
target_compile_features(hello PRIVATE cxx_std_20)
if(MSVC)
  target_compile_options(hello PRIVATE /W4 /permissive-)
else()
  target_compile_options(hello PRIVATE -Wall -Wextra -Wpedantic)
endif()
