#include "common/logging.h"

#include <filesystem>
#include <memory>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

namespace fs = std::filesystem;

namespace common {

void InitLogging(const std::string& log_dir) {
  try {
    if (!log_dir.empty()) fs::create_directories(log_dir);
    const std::string log_path = log_dir.empty() ? "service.log"
                                                 : (log_dir + "/service.log");

    constexpr std::size_t kMaxSize = 10 * 1024 * 1024; // 10 MB
    constexpr std::size_t kMaxFiles = 3;

    auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        log_path, kMaxSize, kMaxFiles);

    auto logger = std::make_shared<spdlog::logger>("service_logger",
                                                   spdlog::sinks_init_list{sink});

    // Pattern: [time] [lvl] thread msg
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [t%t] %v");
    logger->set_level(spdlog::level::info);
    spdlog::set_default_logger(logger);

    spdlog::info("Logging initialized. File: {}", log_path);
  } catch (const std::exception& e) {
    // Fallback to default stderr logger if file logger fails
    spdlog::error("Failed to initialize file logger: {}", e.what());
  }
}

}  // namespace common
