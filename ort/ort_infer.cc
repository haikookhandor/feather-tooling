#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <onnxruntime_cxx_api.h>
#include <spdlog/spdlog.h>
#include "absl/time/clock.h"
#include "absl/time/time.h"

namespace fs = std::filesystem;

static double percentile(std::vector<double>& v, double q) {
  if (v.empty()) return 0.0;
  std::sort(v.begin(), v.end());
  const double idx = q * (static_cast<double>(v.size()) - 1.0);
  const size_t lo = static_cast<size_t>(idx);
  const size_t hi = std::min(lo + 1, v.size() - 1);
  const double frac = idx - static_cast<double>(lo);
  return v[lo] * (1.0 - frac) + v[hi] * frac;
}

static void usage(const char* bin) {
  spdlog::info(
      "Usage:\n"
      "  {} [model.onnx] [runs] [in_dim] [out_dim]\n"
      "Examples:\n"
      "  {}                         # uses models/mlp.onnx, 1000 runs\n"
      "  {} models/mlp.onnx 2000    # 2000 runs, shapes from model\n"
      "  {} models/mlp_128_256_64_static.onnx 5000 128 64  # fallback dims",
      bin, bin, bin, bin);
}

int main(int argc, char** argv) {
  std::string model_path = "models/mlp.onnx";
  int runs = 1000;
  int64_t cli_in_dim  = -1;
  int64_t cli_out_dim = -1;

  if (argc > 1) model_path = argv[1];
  if (argc > 2) runs = std::stoi(argv[2]);
  if (argc > 3) cli_in_dim  = std::stoll(argv[3]);   // e.g., 128
  if (argc > 4) cli_out_dim = std::stoll(argv[4]);   // e.g., 64

  if (!fs::exists(model_path)) {
    spdlog::error("Model not found: {}", model_path);
    usage(argv[0]);
    return 2;
  }

  // ---- ORT env + session ----
  Ort::Env env{ORT_LOGGING_LEVEL_WARNING, "feather-ort"};
  Ort::SessionOptions so;
  so.SetIntraOpNumThreads(1);  // deterministic CPU
  so.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

  Ort::Session session{env, model_path.c_str(), so};
  spdlog::info("Loaded {}", model_path);

  // ---- IO names ----
  const size_t n_inputs  = session.GetInputCount();
  const size_t n_outputs = session.GetOutputCount();
  if (n_inputs != 1 || n_outputs != 1) {
    spdlog::warn("This sample expects 1 input & 1 output, got inputs={}, outputs={}.", n_inputs, n_outputs);
  }

  Ort::AllocatorWithDefaultOptions allocator;

  std::vector<Ort::AllocatedStringPtr> in_name_holders;
  std::vector<Ort::AllocatedStringPtr> out_name_holders;
  std::vector<const char*> in_names, out_names;

  in_name_holders.reserve(n_inputs);
  out_name_holders.reserve(n_outputs);
  in_names.reserve(n_inputs);
  out_names.reserve(n_outputs);

  for (size_t i = 0; i < n_inputs; ++i) {
    in_name_holders.emplace_back(session.GetInputNameAllocated(i, allocator));
    in_names.push_back(in_name_holders.back().get());
  }
  for (size_t i = 0; i < n_outputs; ++i) {
    out_name_holders.emplace_back(session.GetOutputNameAllocated(i, allocator));
    out_names.push_back(out_name_holders.back().get());
  }

  // ---- Try to read shapes from model; if missing, fall back to CLI ----
  std::vector<int64_t> in_shape_vec, out_shape_vec;
  try {
    auto in_type  = session.GetInputTypeInfo(0).GetTensorTypeAndShapeInfo();
    auto out_type = session.GetOutputTypeInfo(0).GetTensorTypeAndShapeInfo();
    in_shape_vec  = in_type.GetShape();    // may be empty if value_info missing
    out_shape_vec = out_type.GetShape();
  } catch (...) {
    // Some providers/models can throw here; we’ll rely on CLI.
  }

  if (in_shape_vec.empty() || out_shape_vec.empty()) {
    if (cli_in_dim <= 0 || cli_out_dim <= 0) {
      spdlog::error("Model shapes unavailable; supply feature dims:");
      usage(argv[0]);
      return 4;
    }
    in_shape_vec  = {1, cli_in_dim};
    out_shape_vec = {1, cli_out_dim};
  } else {
    // Convert dynamic batch (-1) to 1
    if (!in_shape_vec.empty()  && in_shape_vec[0]  < 0) in_shape_vec[0]  = 1;
    if (!out_shape_vec.empty() && out_shape_vec[0] < 0) out_shape_vec[0] = 1;
  }

  if (in_shape_vec.size() != 2 || out_shape_vec.size() != 2) {
    spdlog::error("Unexpected ranks: input rank={}, output rank={}",
                  in_shape_vec.size(), out_shape_vec.size());
    return 3;
  }

  const int64_t in_dim  = in_shape_vec[1];
  const int64_t out_dim = out_shape_vec[1];
  if (in_dim <= 0 || out_dim <= 0) {
    spdlog::error("Non-positive dims resolved: in_dim={}, out_dim={}", in_dim, out_dim);
    return 5;
  }

  // ---- Preallocate buffers & wrap into Ort::Value ----
  Ort::MemoryInfo mem_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

  std::vector<float> in_buf(static_cast<size_t>(in_dim), 0.f);
  std::vector<float> out_buf(static_cast<size_t>(out_dim), 0.f);

  Ort::Value input = Ort::Value::CreateTensor<float>(
      mem_info, in_buf.data(), in_buf.size(),
      in_shape_vec.data(), in_shape_vec.size());
  // NOTE: we let ORT allocate the output and copy back; binding a preallocated
  // output requires IoBinding which we keep out for simplicity.

  // ---- Warm-up (not timed) ----
  {
    for (int i = 0; i < 50; ++i) {
      for (int64_t j = 0; j < in_dim; ++j) in_buf[j] = 0.01f * static_cast<float>((i + j) % 101 - 50);
      (void)session.Run(Ort::RunOptions{nullptr}, in_names.data(), &input, 1, out_names.data(), 1);
    }
  }

  // ---- Single run (print first few logits + latency) ----
  {
    for (int64_t j = 0; j < in_dim; ++j) in_buf[j] = 0.1f * static_cast<float>((j % 5) - 2);

    auto t0 = absl::Now();
    auto outputs = session.Run(Ort::RunOptions{nullptr}, in_names.data(), &input, 1, out_names.data(), 1);
    double ms = absl::ToDoubleMilliseconds(absl::Now() - t0);

    if (!outputs.empty() && outputs[0].IsTensor()) {
      float* p = outputs[0].GetTensorMutableData<float>();
      std::copy(p, p + out_dim, out_buf.begin());
    }

    const int k = static_cast<int>(std::min<int64_t>(5, out_dim));
    std::ostringstream oss; oss << "[";
    for (int i = 0; i < k; ++i) { oss.setf(std::ios::fixed); oss.precision(4); oss << out_buf[i]; if (i + 1 < k) oss << ", "; }
    oss << "]";
    spdlog::info("Single run logits: {}  latency: {:.3f} ms", oss.str(), ms);
  }

  // ---- Timed runs (avg / p50 / p95) ----
  std::mt19937 rng(42);
  std::normal_distribution<float> nd(0.f, 1.f);
  std::vector<double> lat; lat.reserve(static_cast<size_t>(runs));

  for (int i = 0; i < runs; ++i) {
    for (int64_t j = 0; j < in_dim; ++j) in_buf[j] = nd(rng);

    auto t0 = absl::Now();
    auto outputs = session.Run(Ort::RunOptions{nullptr}, in_names.data(), &input, 1, out_names.data(), 1);
    double ms = absl::ToDoubleMilliseconds(absl::Now() - t0);
    lat.push_back(ms);

    // consume one value
    if (!outputs.empty() && outputs[0].IsTensor()) {
      volatile float sink = outputs[0].GetTensorMutableData<float>()[0];
      (void)sink;
    }
  }

  const double avg = std::accumulate(lat.begin(), lat.end(), 0.0) /
                     std::max<size_t>(1, lat.size());
  double p50 = lat.empty() ? 0.0 : percentile(lat, 0.50);
  double p95 = lat.empty() ? 0.0 : percentile(lat, 0.95);

  spdlog::info("Runs: {}  avg: {:.3f} ms  p50: {:.3f} ms  p95: {:.3f} ms",
               runs, avg, p50, p95);

  return 0;
}
