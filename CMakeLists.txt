cmake_minimum_required(VERSION 3.25)
project(FeatherTooling LANGUAGES CXX)

# Default to Release unless specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build types (Debug/Release + custom AddressSanitizer “San”)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel;San" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_SAN "-O1 -g -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS_SAN "-fsanitize=address")
set(CMAKE_SHARED_LINKER_FLAGS_SAN "-fsanitize=address")

# Define NDEBUG in Release (disables assert)
add_compile_definitions($<$<CONFIG:Release>:NDEBUG>)

# FetchContent helper
include(FetchContent)


# fmt (header-only)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 10.2.1
)
FetchContent_MakeAvailable(fmt)

add_subdirectory(app)
add_subdirectory(tests)
