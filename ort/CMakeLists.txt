add_executable(feather_ort ort_infer.cc)
target_link_libraries(feather_ort
  PRIVATE
    onnxruntime::onnxruntime
    spdlog::spdlog
    absl::time
)
target_compile_features(feather_ort PRIVATE cxx_std_20)
