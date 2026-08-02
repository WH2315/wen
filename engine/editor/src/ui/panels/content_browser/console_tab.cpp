#include "ui/panels/content_browser/console_tab.hpp"
#include "ui/icons.hpp"
#include "core/log/logger.hpp"
#include <spdlog/sinks/base_sink.h>
#include <imgui.h>
#include <deque>

namespace wen::editor {

namespace {

struct ConsoleEntry {
    spdlog::level::level_enum level;
    std::string text;
};

struct ConsoleStore {
    std::mutex mutex;
    std::deque<ConsoleEntry> entries;
    int warn_count = 0;
    int error_count = 0;

    static constexpr size_t kMaxEntries = 1000;

    void push(spdlog::level::level_enum level, std::string text) {
        std::lock_guard lock(mutex);
        if (level == spdlog::level::warn) {
            warn_count++;
        } else if (level >= spdlog::level::err) {
            error_count++;
        }
        entries.push_back({level, std::move(text)});
        while (entries.size() > kMaxEntries) {
            entries.pop_front();
        }
    }

    void clear() {
        std::lock_guard lock(mutex);
        entries.clear();
        warn_count = 0;
        error_count = 0;
    }
};

ConsoleStore& store() {
    static ConsoleStore instance;
    return instance;
}

// 向控制台存储写入的 spdlog sink(日志可能来自固定步长线程，因此存储用互斥锁保护)
class ConsoleSink : public spdlog::sinks::base_sink<std::mutex> {
protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        formatter_->format(msg, formatted);
        auto text = fmt::to_string(formatted);
        // 去掉格式化器追加的末尾换行
        while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) {
            text.pop_back();
        }
        store().push(msg.level, std::move(text));
    }

    void flush_() override {}
};

ImVec4 levelColor(spdlog::level::level_enum level) {
    switch (level) {
        case spdlog::level::trace:
        case spdlog::level::debug:
            return {0.55f, 0.55f, 0.58f, 1.0f};
        case spdlog::level::warn:
            return {0.95f, 0.75f, 0.20f, 1.0f};
        case spdlog::level::err:
        case spdlog::level::critical:
            return {0.95f, 0.35f, 0.35f, 1.0f};
        default:
            return {0.85f, 0.85f, 0.85f, 1.0f};
    }
}

}  // namespace

ConsoleTab::ConsoleTab() {
    auto sink = std::make_shared<ConsoleSink>();
    sink->set_pattern("[%H:%M:%S] %v");
    core_logger->addSink(sink);
    client_logger->addSink(sink);
}

void ConsoleTab::render() {
    auto& console = store();

    if (ImGui::Button("Clear")) {
        console.clear();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &auto_scroll_);

    // 右对齐的警告/错误计数
    {
        std::lock_guard lock(console.mutex);
        char counters[64];
        snprintf(counters, sizeof(counters), "%s %d   %s %d",
                 icons::kWarning, console.warn_count, icons::kError, console.error_count);
        float width = ImGui::CalcTextSize(counters).x;
        ImGui::SameLine();
        float pen_x = ImGui::GetCursorPosX();
        float right_x = pen_x + ImGui::GetContentRegionAvail().x - width;
        ImGui::SameLine(std::max(pen_x, right_x));
        ImGui::TextColored(levelColor(spdlog::level::warn), "%s %d", icons::kWarning, console.warn_count);
        ImGui::SameLine();
        ImGui::TextColored(levelColor(spdlog::level::err), "%s %d", icons::kError, console.error_count);
    }

    ImGui::Separator();

    ImGui::BeginChild("##console_log", ImVec2(0.0f, 0.0f), 0, ImGuiWindowFlags_HorizontalScrollbar);
    {
        std::lock_guard lock(console.mutex);
        for (const auto& entry : console.entries) {
            ImGui::TextColored(levelColor(entry.level), "%s", entry.text.c_str());
        }
    }
    if (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 4.0f) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
}

}  // namespace wen::editor
