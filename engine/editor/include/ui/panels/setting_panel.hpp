#pragma once

#include "ui/panel.hpp"

namespace wen::editor {

// 设置:渲染统计、编辑器相机参数与调试开关。
class SettingPanel : public Panel {
public:
    void render() override;
};

}  // namespace wen::editor
