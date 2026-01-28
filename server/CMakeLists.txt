add_executable(feather_server server.cc)
target_link_libraries(feather_server
  PRIVATE
    event_proto
    spdlog::spdlog
    absl::time
    gRPC::grpc++   
    prometheus-cpp::core
    prometheus-cpp::pull
)
target_compile_features(feather_server PRIVATE cxx_std_20)
