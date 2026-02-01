#include "core/log/logger.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace wen {

static spdlog::level::level_enum standardization(const LogLevel level) {
    switch (level) {
        case LogLevel::trace:
            return spdlog::level::level_enum::trace;
        case LogLevel::debug:
            return spdlog::level::level_enum::debug;
        case LogLevel::info:
            return spdlog::level::level_enum::info;
        case LogLevel::warn:
            return spdlog::level::level_enum::warn;
        case LogLevel::error:
            return spdlog::level::level_enum::err;
        case LogLevel::fatal:
            return spdlog::level::level_enum::critical;
        default:
            return {};
    }
}

Logger::Logger(const std::string& name, const LogLevel& level) {
    logger_ = spdlog::stdout_color_mt(name);
    logger_->set_level(standardization(level));
}

Logger::~Logger() {
    spdlog::drop(logger_->name());
    logger_.reset();
}

void Logger::setLevel(const LogLevel& level) const {
    logger_->set_level(standardization(level));
}

}  // namespace wen