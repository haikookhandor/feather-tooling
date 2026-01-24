#include <memory>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>
#include "absl/time/clock.h"
#include "absl/time/time.h"

#include "event.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

namespace feather {

class IngestService final : public Ingest::Service {
 public:
  explicit IngestService(absl::Duration work_delay) : work_delay_(work_delay) {}

  Status Ingest(ServerContext* ctx, const Event* req, Ack* resp) override {
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
    spdlog::info("Server: replying Ack(ok=true)");
    return Status::OK;
  }

 private:
  absl::Duration work_delay_;
};

}  // namespace feather

int main(int argc, char** argv) {
  // Optional: allow a sleep duration in ms to force timeouts for demo
  int sleep_ms = 0;
  if (argc > 1) sleep_ms = std::stoi(argv[1]);

  std::string addr = "0.0.0.0:50051";
  spdlog::info("Starting server on {} (sleep_ms={})", addr, sleep_ms);

  grpc::EnableDefaultHealthCheckService(true);
  // grpc::reflection::InitProtoReflectionServerBuilderPlugin();

  feather::IngestService service(absl::Milliseconds(sleep_ms));

  ServerBuilder builder;
  builder.AddListeningPort(addr, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);

  std::unique_ptr<Server> server(builder.BuildAndStart());
  spdlog::info("Server listening... Ctrl-C to stop.");
  server->Wait();
  return 0;
}
