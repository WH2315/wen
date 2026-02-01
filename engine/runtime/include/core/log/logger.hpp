#pragma once

#include <spdlog/spdlog.h>

namespace wen {

enum class LogLevel : uint8_t { trace, debug, info, warn, error, fatal };

class Logger final {
public:
    Logger(const std::string& name, const LogLevel& level);
    ~Logger();

    void setLevel(const LogLevel& level) const;

    template <typename... Args>
    void log(const LogLevel level, const std::string_view fmt, Args&&... args) {
        auto message =
            fmt::format(fmt::runtime(fmt), std::forward<Args>(args)...);
        switch (level) {
            case LogLevel::trace:
                logger_->trace(message);
                break;
            case LogLevel::debug:
                logger_->debug(message);
                break;
            case LogLevel::info:
                logger_->info(message);
                break;
            case LogLevel::warn:
                logger_->warn(message);
                break;
            case LogLevel::error:
                logger_->error(message);
                break;
            case LogLevel::fatal:
                logger_->critical(message);
                break;
            default:
                break;
        }
    }

private:
    std::shared_ptr<spdlog::logger> logger_;
};

extern Logger* core_logger;
extern Logger* client_logger;

}  // namespace wen

#define __FILENAME__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define WEN_CORE_LOG_HELPER(level, ...)                                  \
    wen::core_logger->log(level, "[" + std::string(__FILENAME__) + ":" + \
                                     std::to_string(__LINE__) + "] " +   \
                                     __VA_ARGS__);
#define WEN_CLIENT_LOG_HELPER(level, ...)                                  \
    wen::client_logger->log(level, "[" + std::string(__FILENAME__) + ":" + \
                                       std::to_string(__LINE__) + "] " +   \
                                       __VA_ARGS__);