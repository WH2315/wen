#pragma once

#include <filesystem>
#include <string>

namespace wen::editor {

// 引擎内文件浏览器对话框(打开场景 / 另存为场景)。
class FileDialog {
public:
    enum class Mode {
        eOpen,  // 打开:列表内单击选中、双击直接确认
        eSave,  // 另存为:以文件名输入框内容为准(双击仅填名,防误覆盖)
    };

    void open(Mode mode, const std::string& title, const std::filesystem::path& initial_dir,
              const std::string& filter_ext = ".scene");

    void setDefaultFileName(const std::string& name);

    void close();
    bool isOpen() const { return open_; }
    Mode mode() const { return mode_; }

    void render();

    bool takeResult(bool& confirmed, std::filesystem::path& path);

private:
    void renderNavigation();
    void renderFileList();
    bool isWithinRoot(const std::filesystem::path& path) const;
    std::filesystem::path buildResultPath() const;

    bool open_ = false;
    Mode mode_ = Mode::eOpen;
    std::string title_ = "Open Scene";
    std::string filter_ext_ = ".scene";

    std::filesystem::path asset_root_{"engine/assets"};
    std::filesystem::path current_dir_;
    std::filesystem::path selected_path_;

    char file_name_[256] = {0};
    char default_file_name_[256] = {0};

    bool confirm_requested_ = false;
    bool focus_file_name_ = false;

    bool result_pending_ = false;
    bool result_confirmed_ = false;
    std::filesystem::path result_path_;
};

}  // namespace wen::editor
