add_library(common
  common/logging.cc
  common/status_demo.cc
)

target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(common
  PUBLIC
    spdlog::spdlog
    absl::status
    absl::statusor
    absl::time
)

target_compile_features(common PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(common PRIVATE /W4 /permissive-)
else()
  target_compile_options(common PRIVATE -Wall -Wextra -Wpedantic)
endif()
