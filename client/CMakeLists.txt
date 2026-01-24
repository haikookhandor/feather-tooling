add_executable(feather_client client.cc)
target_link_libraries(feather_client
  PRIVATE
    event_proto
    spdlog::spdlog
    absl::time
)
target_compile_features(feather_client PRIVATE cxx_std_20)
