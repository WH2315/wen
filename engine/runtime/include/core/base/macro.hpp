#pragma once

#include "core/log/logger.hpp"

#define WEN_CORE_TRACE(...) WEN_CORE_LOG_HELPER(wen::LogLevel::trace, __VA_ARGS__)
#define WEN_CORE_DEBUG(...) WEN_CORE_LOG_HELPER(wen::LogLevel::debug, __VA_ARGS__)
#define WEN_CORE_INFO(...) WEN_CORE_LOG_HELPER(wen::LogLevel::info, __VA_ARGS__)
#define WEN_CORE_WARN(...) WEN_CORE_LOG_HELPER(wen::LogLevel::warn, __VA_ARGS__)
#define WEN_CORE_ERROR(...) WEN_CORE_LOG_HELPER(wen::LogLevel::error, __VA_ARGS__)
#define WEN_CORE_FATAL(...) WEN_CORE_LOG_HELPER(wen::LogLevel::fatal, __VA_ARGS__)

#define WEN_CLIENT_TRACE(...) WEN_CLIENT_LOG_HELPER(wen::LogLevel::trace, __VA_ARGS__)
#define WEN_CLIENT_DEBUG(...) WEN_CLIENT_LOG_HELPER(wen::LogLevel::debug, __VA_ARGS__)
#define WEN_CLIENT_INFO(...) WEN_CLIENT_LOG_HELPER(wen::LogLevel::info, __VA_ARGS__)
#define WEN_CLIENT_WARN(...) WEN_CLIENT_LOG_HELPER(wen::LogLevel::warn, __VA_ARGS__)
#define WEN_CLIENT_ERROR(...) WEN_CLIENT_LOG_HELPER(wen::LogLevel::error, __VA_ARGS__)
#define WEN_CLIENT_FATAL(...) WEN_CLIENT_LOG_HELPER(wen::LogLevel::fatal, __VA_ARGS__)

#define WEN_CORE_ASSERT(x, ...)                               \
    if (!(x)) {                                               \
        WEN_CORE_FATAL("Assertion Failed: {0}", __VA_ARGS__); \
        std::abort();                                         \
    }

#define WEN_CLIENT_ASSERT(x, ...)                               \
    if (!(x)) {                                                 \
        WEN_CLIENT_FATAL("Assertion Failed: {0}", __VA_ARGS__); \
        std::abort();                                           \
    }