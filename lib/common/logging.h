#pragma once
#include <string>

namespace common {

// Initializes a rotating spdlog sink at logs/service.log (10 MB, 3 files).
// Also sets a nice pattern and makes it the default logger.
void InitLogging(const std::string& log_dir = "logs");

}  // namespace common
