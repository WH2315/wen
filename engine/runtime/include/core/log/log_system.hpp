#pragma once

#include "core/base/singleton.hpp"
#include "core/log/logger.hpp"

namespace wen {

class LogSystem final {
    friend class Singleton<LogSystem>;
    LogSystem(LogLevel core_level, LogLevel client_level);
    ~LogSystem();

public:
    Logger* createLogger(const std::string& name, LogLevel level);

    static void setCoreLevel(const LogLevel& level) {
        core_logger->setLevel(level);
    }

    static void setClientLevel(const LogLevel& level) {
        client_logger->setLevel(level);
    }

private:
    std::vector<std::unique_ptr<Logger>> loggers_;
};

}  // namespace wen