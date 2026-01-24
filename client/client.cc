#include <chrono>
#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include <spdlog/spdlog.h>
#include "absl/time/clock.h"
#include "absl/time/time.h"

#include "event.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

int main(int argc, char** argv) {
  // Args: [deadline_ms]
  //       If provided (e.g., 20), the client will set a deadline 20ms in the future.
  int deadline_ms = -1;
  if (argc > 1) deadline_ms = std::stoi(argv[1]);

  auto channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
  auto stub = feather::Ingest::NewStub(channel);

  feather::Event ev;
  ev.set_id("abc");
  ev.set_val(3.14);
  ev.set_ts(absl::ToUnixMillis(absl::Now()));

  feather::Ack ack;
  ClientContext ctx;

  if (deadline_ms >= 0) {
    auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(deadline_ms);
    ctx.set_deadline(deadline);
    spdlog::info("Client: using deadline={} ms", deadline_ms);
  } else {
    spdlog::info("Client: no deadline");
  }

  Status status = stub->Ingest(&ctx, ev, &ack);

  if (status.ok()) {
    spdlog::info("Client: success ok={} msg='{}'", ack.ok(), ack.msg());
  } else {
    spdlog::warn("Client: RPC failed code={} message='{}'",
                 static_cast<int>(status.error_code()),
                 status.error_message());
    if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED) {
      spdlog::warn("Client: deadline exceeded (timeout)");
    }
  }

  return status.ok() ? 0 : 1;
}
