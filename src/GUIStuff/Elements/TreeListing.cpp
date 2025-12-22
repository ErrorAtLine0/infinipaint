#include "TreeListing.hpp"
#include "../GUIManager.hpp"
#include "Helpers/ConvertVec.hpp"

namespace GUIStuff {

void TreeListing::update(UpdateInputData& io, GUIManager& gui, NetworkingObjects::NetObjID rootObjID, const DisplayData& displayData) {
    if(!io.mouse.leftHeld)
        dragHoldSelected = false;

    orderedObjsSelected.clear();
    std::optional<ParentObjectIDStack> hoveredObject;

    CLAY_AUTO_ID({
        .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}
    }) {
        Clay_ElementId localID = CLAY_ID_LOCAL("scroll area");
        float setScrollAmount;
        CLAY(localID, {
            .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}
        }) {
            gui.scroll_bar_area("scroll area", true, [&](float scrollContentHeight, float containerHeight, float& scrollAmount) {
                setScrollAmount = scrollAmount;
                size_t itemCount = 0;
                size_t itemCountToStartAt = -scrollAmount / ENTRY_HEIGHT;
                size_t itemCountToEndAt = (containerHeight - scrollAmount) / ENTRY_HEIGHT;
                itemCountToEndAt++;
                std::vector<NetworkingObjects::NetObjID> parentIDStack;
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(ENTRY_HEIGHT * itemCountToStartAt)}
                    }
                }) {}
                recursive_gui(io, gui, displayData, rootObjID, itemCount, itemCountToStartAt, itemCountToEndAt, 0, hoveredObject, parentIDStack, false);
                if(itemCount > itemCountToEndAt) {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(ENTRY_HEIGHT * (itemCount - itemCountToEndAt))}
                        }
                    }) {}
                }
            });
        }
        Clay_ElementData scrollAreaBB = Clay_GetElementData(localID);
        if(scrollAreaBB.found) {
            float mouseRelativeYPos = std::fabs(io.mouse.pos.y() - scrollAreaBB.boundingBox.y - setScrollAmount);
            topHalfOfHovered = std::fmod(mouseRelativeYPos, ENTRY_HEIGHT) < (ENTRY_HEIGHT / 2.0f);
        }
    }

    // Any objects that were part of a collapsed directory will be unselected
    objsSelected.clear();
    for(auto& [parent, object] : orderedObjsSelected)
        objsSelected.emplace(object);

    if(objDragged.has_value()) {
        ParentObjectIDPair& objDraggedVal = objDragged.value();
        if(!objsSelected.contains(objDraggedVal.object))
            objDragged = std::nullopt;
        else if(!io.mouse.leftHeld) {
            if(!hoveredObject.has_value())
                set_single_selected_object();
            else {
                ParentObjectIDStack hoveredObjValToUse = hoveredObject.value(); // By value since it'll be edited
                size_t indexToMoveFrom = displayData.getIndexOfObjInList(objDraggedVal);
                size_t indexToMoveTo = displayData.getIndexOfObjInList({hoveredObjValToUse.parents.back(), hoveredObjValToUse.object});
                std::optional<DisplayData::ObjInList> objInList = displayData.getObjInListAtIndex(hoveredObjValToUse.parents.back(), indexToMoveTo);
                if(objInList.value().isDirectoryOpen && !topHalfOfHovered) { // Move to front of the list object if it's open
                    hoveredObjValToUse.parents.emplace_back(hoveredObjValToUse.object);
                    indexToMoveTo = 0;
                }
                else if(!topHalfOfHovered)
                    indexToMoveTo++;
                if((indexToMoveFrom == indexToMoveTo || (indexToMoveFrom + 1) == indexToMoveTo) && objDraggedVal.parent == hoveredObjValToUse.parents.back())
                    set_single_selected_object();
                else {
                    bool wrongMovement = false;
                    // Check if object is about to move into itself
                    for(const NetworkingObjects::NetObjID& parent : hoveredObjValToUse.parents) {
                        if(objsSelected.contains(parent)) {
                            wrongMovement = true;
                            break;
                        }
                    }
                    if(!wrongMovement)
                        displayData.moveObjectsToListAtIndex(hoveredObjValToUse.parents.back(), indexToMoveTo, orderedObjsSelected);
                    objsSelected.clear();
                    orderedObjsSelected.clear();
                }
            }
            objDragged = std::nullopt;
        }
    }

    oldHoveredObject = hoveredObject;
}

void TreeListing::set_single_selected_object() {
    objsSelected.clear();
    orderedObjsSelected.clear();
    objsSelected.emplace(objDragged.value().object);
    orderedObjsSelected.emplace_back(objDragged.value());
}

void TreeListing::recursive_gui(UpdateInputData& io, GUIManager& gui, const DisplayData& displayData, NetworkingObjects::NetObjID objID, size_t& itemCount, size_t itemCountToStartAt, size_t itemCountToEndAt, size_t listDepth, std::optional<ParentObjectIDStack>& hoveredObject, std::vector<NetworkingObjects::NetObjID>& parentIDStack, bool parentJustSelected) {
    parentIDStack.emplace_back(objID);
    for(size_t i = 0;; i++) {
        std::optional<DisplayData::ObjInList> objInListOptional = displayData.getObjInListAtIndex(objID, i);
        if(!objInListOptional.has_value())
            break;
        else {
            auto& objInList = objInListOptional.value();
            bool selfJustSelected = false;
            if(itemCount >= itemCountToStartAt && itemCount < itemCountToEndAt) {
                gui.push_id(itemCount);
                bool buttonClicked = false;
                uint16_t topOutlineWidth = 0;
                uint16_t bottomOutlineWidth = 0;
                if(oldHoveredObject.has_value() && oldHoveredObject.value().object == objInList.id) {
                    if(objDragged.has_value()) {
                        ParentObjectIDPair& objDraggedVal = objDragged.value();
                        if(objDraggedVal.object != objInList.id) {
                            if(topHalfOfHovered)
                                topOutlineWidth = 1;
                            else
                                bottomOutlineWidth = 1;
                        }
                    }
                }
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(ENTRY_HEIGHT)},
                        .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                    },
                    .backgroundColor = convert_vec4<Clay_Color>((objsSelected.contains(objInList.id)) ? io.theme->backColor1 : io.theme->backColor2)
                }) {
                    if(listDepth != 0) {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_FIXED(ENTRY_HEIGHT * listDepth), .height = CLAY_SIZING_FIXED(ENTRY_HEIGHT)}
                            },
                        }) {}
                    }
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}
                        },
                        .border = {
                            .color = convert_vec4<Clay_Color>(io.theme->frontColor1),
                            .width = {
                                .left = 0,
                                .right = 0,
                                .top = topOutlineWidth,
                                .bottom = bottomOutlineWidth,
                                .betweenChildren = 0
                            }
                        }
                    }) {
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
                            buttonClicked |= displayData.drawObjGUI({objID, objInList.id});
                        }
                    }
                
                    if(Clay_Hovered()) {
                        hoveredObject = {parentIDStack, objInList.id};
                        if(io.mouse.leftClick && !buttonClicked && !parentJustSelected) {
                            if(io.key.leftShift)
                                dragHoldSelected = true;
                            else if(io.key.leftCtrl) {
                                bool alreadySelected = objsSelected.contains(objInList.id);
                                if(alreadySelected)
                                    objsSelected.erase(objInList.id);
                                else {
                                    select_and_unselect_parents(parentIDStack, objInList.id);
                                    selfJustSelected = true;
                                }
                            }
                            else {
                                if(!objsSelected.contains(objInList.id)) {
                                    objsSelected.clear();
                                    orderedObjsSelected.clear();
                                    objsSelected.emplace(objInList.id);
                                    selfJustSelected = true;
                                }
                                objDragged = {objID, objInList.id};
                            }
                        }
                        if(io.key.leftShift && dragHoldSelected && !parentJustSelected) {
                            select_and_unselect_parents(parentIDStack, objInList.id);
                            selfJustSelected = true;
                        }
                    }
                }
                gui.pop_id();
            }
            if(!parentJustSelected) {
                if(objsSelected.contains(objInList.id))
                    orderedObjsSelected.emplace_back(objID, objInList.id);
            }
            else
                unselect_object(objInList.id);
            itemCount++;
            if(objInList.isDirectoryOpen)
                recursive_gui(io, gui, displayData, objInList.id, itemCount, itemCountToStartAt, itemCountToEndAt, listDepth + 1, hoveredObject, parentIDStack, selfJustSelected | parentJustSelected);
        }
    }
    parentIDStack.pop_back();
}

void TreeListing::unselect_object(NetworkingObjects::NetObjID id) {
    objsSelected.erase(id);
    std::erase_if(orderedObjsSelected, [&id](const auto& p) {
        return p.object == id;
    });
}

void TreeListing::select_and_unselect_parents(const std::vector<NetworkingObjects::NetObjID>& parentIDStack, NetworkingObjects::NetObjID newSelectedObj) {
    for(const NetworkingObjects::NetObjID& parent : parentIDStack) {
        if(objsSelected.contains(parent)) {
            unselect_object(parent);
            break;
        }
    }
    objsSelected.emplace(newSelectedObj);
}

void TreeListing::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) {
}

}
