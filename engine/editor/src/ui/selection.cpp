#include "ui/selection.hpp"
#include "ui/ui_context.hpp"
#include "engine/global_context.hpp"

namespace wen::editor {

void setSelectedGameObject(GameObjectUUID uuid) {
    global_ui_context->selected_game_object_uuid = uuid;
    global_context->render_system->setSelectedGameObject(uuid);
}

}  // namespace wen::editor
