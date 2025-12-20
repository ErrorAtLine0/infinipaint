#include "TreeListing.hpp"
#include "../GUIManager.hpp"
#include "Helpers/ConvertVec.hpp"

namespace GUIStuff {

void TreeListing::update(UpdateInputData& io, GUIManager& gui, NetworkingObjects::NetObjID rootObjID, const DisplayData& displayData) {
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}
        }
    }) {
        gui.scroll_bar_area("scroll area", true, [&](float scrollContentHeight, float containerHeight, float& scrollAmount) {
            size_t itemCount = 0;
            size_t itemCountToStartAt = -scrollAmount / ENTRY_HEIGHT;
            size_t itemCountToEndAt = (containerHeight - scrollAmount) / ENTRY_HEIGHT;
            itemCountToEndAt++;
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(ENTRY_HEIGHT * itemCountToStartAt)}
                }
            }) {}
            recursive_gui(io, gui, displayData, rootObjID, itemCount, itemCountToStartAt, itemCountToEndAt, 0);
            if(itemCount > itemCountToEndAt) {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(ENTRY_HEIGHT * (itemCount - itemCountToEndAt))}
                    }
                }) {}
            }
        });
    }
}

void TreeListing::recursive_gui(UpdateInputData& io, GUIManager& gui, const DisplayData& displayData, NetworkingObjects::NetObjID objID, size_t& itemCount, size_t itemCountToStartAt, size_t itemCountToEndAt, size_t listDepth) {
    for(size_t i = 0;; i++) {
        std::optional<DisplayData::ObjInList> objInListOptional = displayData.getObjInListAtIndex(objID, i);
        if(!objInListOptional.has_value())
            break;
        else {
            auto& objInList = objInListOptional.value();
            if(itemCount >= itemCountToStartAt && itemCount < itemCountToEndAt) {
                gui.push_id(itemCount);
                bool buttonClicked = false;
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(ENTRY_HEIGHT)},
                        .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                    },
                    .backgroundColor = convert_vec4<Clay_Color>((objSelected.has_value() && objSelected.value() == objInList.id) ? io.theme->backColor1 : io.theme->backColor2),
                }) {
                    if(listDepth != 0) {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_FIXED(ENTRY_HEIGHT * listDepth), .height = CLAY_SIZING_FIXED(ENTRY_HEIGHT)}
                            },
                        }) {}
                    }
                    if(objInList.isDirectory) {
                        if(gui.svg_icon_button_transparent("ico", objInList.isDirectoryOpen ? "data/icons/droparrow.svg" : "data/icons/droparrowclose.svg", false, ENTRY_HEIGHT)) {
                            displayData.setDirectoryOpen(objInList.id, !objInList.isDirectoryOpen);
                            buttonClicked = true;
                        }
                    }
                    else {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_FIXED(ENTRY_HEIGHT), .height = CLAY_SIZING_FIXED(ENTRY_HEIGHT)},
                                .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                            },
                        }) {
                            buttonClicked |= displayData.drawNonDirectoryObjIconGUI(objInList.id);
                        }
                    }
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                        }
                    }) {
                        buttonClicked |= displayData.drawObjGUI(objID, objInList.id);
                    }
                
                    if(Clay_Hovered() && io.mouse.leftClick && !buttonClicked)
                        objSelected = objInList.id;
                }
                gui.pop_id();
            }
            itemCount++;
            if(objInList.isDirectoryOpen)
                recursive_gui(io, gui, displayData, objInList.id, itemCount, itemCountToStartAt, itemCountToEndAt, listDepth + 1);
        }
    }
}

void TreeListing::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) {
}

}
