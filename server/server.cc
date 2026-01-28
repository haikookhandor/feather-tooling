#include <memory>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>
#include "absl/time/clock.h"
#include "absl/time/time.h"

#include "event.grpc.pb.h"

#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/histogram.h>
#include <prometheus/serializer.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

namespace feather {

class IngestService final : public Ingest::Service {
 public:
  explicit IngestService(absl::Duration work_delay, prometheus::Counter& req_counter, prometheus::Histogram& latency_hist) : work_delay_(work_delay), req_counter_(req_counter), latency_hist_(latency_hist) {}

  Status Ingest(ServerContext* ctx, const Event* req, Ack* resp) override {

    const auto t0 = absl::Now();

    // If the client timeout expires, gRPC may cancel the RPC; detect it.
    if (ctx->IsCancelled()) {
      spdlog::warn("Server: request cancelled before start");
      return Status(grpc::StatusCode::CANCELLED, "Cancelled");
    }

    spdlog::info("Server: got Event(id={}, val={}, ts={})",
                 req->id(), req->val(), req->ts());

    // Simulate work; during this sleep the client may time out.
    if (work_delay_ > absl::ZeroDuration()) {
      absl::SleepFor(work_delay_);
    }

    if (ctx->IsCancelled()) {
      spdlog::warn("Server: request cancelled during work (deadline exceeded?)");
      return Status(grpc::StatusCode::CANCELLED, "Cancelled mid-flight");
    }

    resp->set_ok(true);
    resp->set_msg("ok");

    req_counter_.Increment();
    const double ms = absl::ToDoubleMilliseconds(absl::Now()-t0);
    latency_hist_.Observe(ms);

    spdlog::info("Server: reply ok (latency {:.3f} ms)", ms);
    return Status::OK;
  }

 private:
  absl::Duration work_delay_;
  prometheus::Counter&   req_counter_;
  prometheus::Histogram& latency_hist_;
};

}  // namespace feather

int main(int argc, char** argv) {
  // Optional: allow a sleep duration in ms to force timeouts for demo
  int sleep_ms = 0;
  if (argc > 1) sleep_ms = std::stoi(argv[1]);

  // --- Prometheus registry + exposer on :8080 ---
  auto registry = std::make_shared<prometheus::Registry>();
  // Families
  auto& reqs_family = prometheus::BuildCounter()
                          .Name("ingest_requests_total")
                          .Help("Total number of ingest RPCs received")
                          .Register(*registry);
  auto& lat_family  = prometheus::BuildHistogram()
                          .Name("ingest_latency_ms")
                          .Help("Ingest RPC latency in milliseconds")
                          .Register(*registry);

  // pick ms buckets that align with your SLOs
  const std::vector<double> kLatencyMsBuckets{
      0.5, 1, 2, 5, 10, 20, 50, 100, 200, 500, 1000
  };

  // Single unlabeled metrics for now
  auto& reqs_counter = reqs_family.Add({});
  auto& lat_hist     = lat_family.Add({}, kLatencyMsBuckets);

  // Expose /metrics on 0.0.0.0:8080 (Prometheus pull)
  prometheus::Exposer exposer{"0.0.0.0:8080"};
  exposer.RegisterCollectable(registry);

  std::string addr = "0.0.0.0:50051";
  spdlog::info("Starting server on {} (sleep_ms={})", addr, sleep_ms);
  spdlog::info("Prometheus /metrics on 0.0.0.0:8080");

  feather::IngestService service(absl::Milliseconds(sleep_ms), reqs_counter, lat_hist);


  grpc::EnableDefaultHealthCheckService(true);
  // grpc::reflection::InitProtoReflectionServerBuilderPlugin();


  ServerBuilder builder;
  builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  spdlog::info("Server listening... Ctrl-C to stop.");
  server->Wait();
  return 0;
}
