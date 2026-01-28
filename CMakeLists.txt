cmake_minimum_required(VERSION 3.25)
project(FeatherTooling LANGUAGES CXX)
# Tell CMake where Homebrew packages live (Apple Silicon)
list(PREPEND CMAKE_PREFIX_PATH "/opt/homebrew")

# Find gRPC (exports gRPC::grpc++, gRPC::grpc_cpp_plugin, etc.)
find_package(gRPC CONFIG REQUIRED)

# Homebrew Abseil
find_package(absl CONFIG REQUIRED)
# Prometheus C++ client
find_package(prometheus-cpp CONFIG REQUIRED)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel;San" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_SAN "-O1 -g -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS_SAN "-fsanitize=address")
set(CMAKE_SHARED_LINKER_FLAGS_SAN "-fsanitize=address")

add_compile_definitions($<$<CONFIG:Release>:NDEBUG>)

include(FetchContent)

# fmt (external)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 10.2.1
)
FetchContent_MakeAvailable(fmt)

# >>> ADD THESE TWO LINES <<<
set(SPDLOG_FMT_EXTERNAL ON  CACHE BOOL "Use external fmt" FORCE)
set(SPDLOG_USE_STD_FORMAT OFF CACHE BOOL "Disable std::format" FORCE)

# spdlog (built against external fmt)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.13.0
)
FetchContent_MakeAvailable(spdlog)

# Google Benchmark
FetchContent_Declare(
  gbench
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.8.3
)
# Disable its internal tests to speed up configure
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(gbench)

add_subdirectory(proto)
add_subdirectory(server)
add_subdirectory(client)

# Our benchmarks
add_subdirectory(bench)

add_subdirectory(app)
add_subdirectory(tests)
add_subdirectory(lib)
