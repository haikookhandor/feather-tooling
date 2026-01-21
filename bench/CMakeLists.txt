add_executable(bench_accumulate bench_accumulate.cc)

target_link_libraries(bench_accumulate
  PRIVATE
    benchmark::benchmark
    benchmark::benchmark_main
)

# Use the same warnings style
target_compile_features(bench_accumulate PRIVATE cxx_std_20)
if(MSVC)
  target_compile_options(bench_accumulate PRIVATE /W4 /permissive-)
else()
  target_compile_options(bench_accumulate PRIVATE -Wall -Wextra -Wpedantic -O3)
endif()
