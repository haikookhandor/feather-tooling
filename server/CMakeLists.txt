add_executable(feather_server server.cc)
target_link_libraries(feather_server
  PRIVATE
    event_proto
    spdlog::spdlog
    absl::time
    gRPC::grpc++   
)
target_compile_features(feather_server PRIVATE cxx_std_20)
