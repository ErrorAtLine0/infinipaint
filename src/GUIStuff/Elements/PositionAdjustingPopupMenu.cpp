#include "PositionAdjustingPopupMenu.hpp"
#include "../GUIManager.hpp"
#include "Helpers/ConvertVec.hpp"

namespace GUIStuff {

PositionAdjustingPopupMenu::PositionAdjustingPopupMenu(GUIManager& gui): Element(gui) {}

void PositionAdjustingPopupMenu::layout(const Clay_ElementId& id, Vector2f popupPos, const std::function<void()>& innerContent) {
    if(layoutElement && layoutElement->get_bb().has_value()) {
        auto& bb = layoutElement->get_bb().value();
        if((popupPos.y() + bb.height()) > gui.io.windowSize.y())
            popupPos.y() -= bb.height();
        if((popupPos.x() + bb.width()) > gui.io.windowSize.x())
            popupPos.x() -= bb.width();
    }

    layoutElement = gui.element<LayoutElement>("popup", [&] (const Clay_ElementId& lId) {
        CLAY(lId, {
            .layout = { 
                .sizing = {.width = CLAY_SIZING_FIT(100), .height = CLAY_SIZING_FIT(0)},
                .padding = CLAY_PADDING_ALL(gui.io.theme->padding1),
                .childGap = 1,
                .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            },
            .backgroundColor = convert_vec4<Clay_Color>(gui.io.theme->backColor1),
            .cornerRadius = CLAY_CORNER_RADIUS(gui.io.theme->windowCorners1),
            .floating = {
                .offset = {popupPos.x(), popupPos.y()},
                .zIndex = gui.get_z_index(),
                .attachTo = CLAY_ATTACH_TO_ROOT
            }
        }) {
            innerContent();
        }
    });
}

}
