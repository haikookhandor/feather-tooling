# proto/CMakeLists.txt
# Only ask for gRPC; it will bring Protobuf in via find_dependency().
find_package(gRPC CONFIG REQUIRED)

set(PROTO_SRC ${CMAKE_CURRENT_SOURCE_DIR}/event.proto)

# Where generated sources will live
set(GEN_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GEN_SRC_DIR})

# Use the imported targets to locate the tools (no LOCATION property needed).
set(PROTOC_EXEC         $<TARGET_FILE:protobuf::protoc>)
set(GRPC_CPP_PLUGIN_BIN $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

# --- Protobuf (message) codegen ---
add_custom_command(
  OUTPUT
    ${GEN_SRC_DIR}/event.pb.cc
    ${GEN_SRC_DIR}/event.pb.h
  COMMAND ${PROTOC_EXEC}
  ARGS --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
       --cpp_out=${GEN_SRC_DIR}
       ${PROTO_SRC}
  DEPENDS ${PROTO_SRC}
  VERBATIM)

# --- gRPC (service) codegen ---
add_custom_command(
  OUTPUT
    ${GEN_SRC_DIR}/event.grpc.pb.cc
    ${GEN_SRC_DIR}/event.grpc.pb.h
  COMMAND ${PROTOC_EXEC}
  ARGS --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
       --grpc_out=${GEN_SRC_DIR}
       --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN_BIN}
       ${PROTO_SRC}
  DEPENDS ${PROTO_SRC} ${GEN_SRC_DIR}/event.pb.h
  VERBATIM)

add_library(event_proto
  ${GEN_SRC_DIR}/event.pb.cc
  ${GEN_SRC_DIR}/event.grpc.pb.cc)

target_include_directories(event_proto PUBLIC ${GEN_SRC_DIR})

# Link against gRPC and Protobuf libs that gRPC already found.
target_link_libraries(event_proto
  PUBLIC
    gRPC::grpc++
    protobuf::libprotobuf
)

target_compile_features(event_proto PUBLIC cxx_std_20)
