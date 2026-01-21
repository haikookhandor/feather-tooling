#include <benchmark/benchmark.h>
#include <numeric>
#include <random>
#include <vector>

// Prepare a 1M-element vector of floats.
static std::vector<float> make_data() {
  std::vector<float> v(1'000'000);
  std::mt19937 rng(42);
  std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
  for (auto& x : v) x = dist(rng);
  return v;
}

static void BM_Accumulate(benchmark::State& st) {
  const static std::vector<float> data = make_data();
  for (auto _ : st) {
    float s = std::accumulate(data.begin(), data.end(), 0.0f);
    benchmark::DoNotOptimize(s);
  }
  st.SetItemsProcessed(static_cast<int64_t>(st.iterations()) * data.size());
}

static void BM_ManualLoop(benchmark::State& st) {
  const static std::vector<float> data = make_data();
  for (auto _ : st) {
    float s = 0.0f;
    // Manual loop: a common baseline when you start optimizing.
    for (size_t i = 0, n = data.size(); i < n; ++i) {
      s += data[i];
    }
    benchmark::DoNotOptimize(s);
  }
  st.SetItemsProcessed(static_cast<int64_t>(st.iterations()) * data.size());
}

BENCHMARK(BM_Accumulate);
BENCHMARK(BM_ManualLoop);
BENCHMARK_MAIN();
