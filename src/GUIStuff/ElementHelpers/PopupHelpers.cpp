#include "PopupHelpers.hpp"

namespace GUIStuff { namespace ElementHelpers {

void paint_circle_popup_menu(GUIManager& gui, const char* id, const Vector2f& centerPos, const PaintCircleMenu::Data& val) {
    constexpr float size = 300.0f;
    gui.new_id(id, [&] {
        CLAY_AUTO_ID({.layout = { 
                .sizing = {.width = CLAY_SIZING_FIXED(size), .height = CLAY_SIZING_FIXED(size)}
            },
            .floating = {
                .offset = {centerPos.x() - size * 0.5f, centerPos.y() - size * 0.5f},
                .zIndex = gui.get_z_index(),
                .attachTo = CLAY_ATTACH_TO_ROOT
            }
        }) {
            gui.element<PaintCircleMenu>("paint circle menu", val);
        }
    });
}

}}
