#include "TreeListing.hpp"
#include "../GUIManager.hpp"
#include "Helpers/ConvertVec.hpp"
#include <chrono>
#include "../../TimePoint.hpp"

#define TIME_TO_HOVER_TO_OPEN_DIRECTORY std::chrono::milliseconds(700)

namespace GUIStuff {

void TreeListing::update(UpdateInputData& io, GUIManager& gui, NetworkingObjects::NetObjID rootObjID, const DisplayData& displayData, SelectionData& selectionData) {
    if(!io.mouse.leftHeld)
        dragHoldSelected = false;

    selectionData.orderedObjsSelected.clear();
    std::optional<ParentObjectIDStack> hoveredObject;

    CLAY_AUTO_ID({
        .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}
    }) {
        Clay_ElementId localID = CLAY_ID_LOCAL("scroll area");
        float setScrollAmount;
        float setScrollContentHeight;
        CLAY(localID, {
            .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}
        }) {
            gui.scroll_bar_area("scroll area", true, [&](float scrollContentHeight, float containerHeight, float& scrollAmount) {
                setScrollAmount = scrollAmount;
                setScrollContentHeight = scrollContentHeight;
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
                recursive_gui(io, gui, displayData, selectionData, rootObjID, itemCount, itemCountToStartAt, itemCountToEndAt, 0, hoveredObject, parentIDStack, false);
                if(itemCount > itemCountToEndAt) {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(ENTRY_HEIGHT * (itemCount - itemCountToEndAt))}
                        }
                    }) {}
                }
            }, false);
        }
        Clay_ElementData scrollAreaBB = Clay_GetElementData(localID);
        if(scrollAreaBB.found) {
            // Clamp by fabs(scrollcontentheight - 3), small offset is added to account for error (when youre at the very edge it might assume youre at the top of an element)
            float mouseRelativeYPos = std::clamp(std::fabs(io.mouse.pos.y() - scrollAreaBB.boundingBox.y - setScrollAmount), 0.0f, std::fabs(setScrollContentHeight - 3.0f));
            topHalfOfHovered = std::fmod(mouseRelativeYPos, ENTRY_HEIGHT) < (ENTRY_HEIGHT / 2.0f);
        }
    }

    // Any objects that were part of a collapsed directory will be unselected
    selectionData.objsSelected.clear();
    for(auto& [parent, object] : selectionData.orderedObjsSelected)
        selectionData.objsSelected.emplace(object);

    if(objDragged.has_value()) {
        if(hoveredObject.has_value() && !selectionData.objsSelected.contains(hoveredObject.value().object) && (std::chrono::steady_clock::now() - timeStartedHoveringOverObject) >= TIME_TO_HOVER_TO_OPEN_DIRECTORY) {
            ParentObjectIDStack& hoveredObjVal = hoveredObject.value();
            size_t hoveredObjIndex = displayData.getIndexOfObjInList({hoveredObjVal.parents.back(), hoveredObjVal.object});
            std::optional<DisplayData::ObjInList> objHoveringOver = displayData.getObjInListAtIndex(hoveredObjVal.parents.back(), hoveredObjIndex);
            if(objHoveringOver.value().isDirectory && !objHoveringOver.value().isDirectoryOpen)
                displayData.setDirectoryOpen(hoveredObjVal.object, true);
        }

        ParentObjectIDPair& objDraggedVal = objDragged.value();
        if(!selectionData.objsSelected.contains(objDraggedVal.object))
            objDragged = std::nullopt;
        else if(!io.mouse.leftHeld) {
            if(!hoveredObject.has_value())
                set_single_selected_object(selectionData);
            else {
                ParentObjectIDStack hoveredObjValToUse = hoveredObject.value(); // By value since it'll be edited
                size_t indexToMoveFrom = displayData.getIndexOfObjInList(objDraggedVal);
                size_t indexToMoveTo = displayData.getIndexOfObjInList({hoveredObjValToUse.parents.back(), hoveredObjValToUse.object});
                std::optional<DisplayData::ObjInList> objHoveringOver = displayData.getObjInListAtIndex(hoveredObjValToUse.parents.back(), indexToMoveTo);
                if(objHoveringOver.value().isDirectoryOpen && !topHalfOfHovered) { // Move to front of the directory object
                    hoveredObjValToUse.parents.emplace_back(hoveredObjValToUse.object);
                    indexToMoveTo = 0;
                }
                else if(!topHalfOfHovered)
                    indexToMoveTo++;
                if((indexToMoveFrom == indexToMoveTo || (indexToMoveFrom + 1) == indexToMoveTo) && objDraggedVal.parent == hoveredObjValToUse.parents.back())
                    set_single_selected_object(selectionData);
                else {
                    bool wrongMovement = false;
                    // Check if object is about to move into itself
                    for(const NetworkingObjects::NetObjID& parent : hoveredObjValToUse.parents) {
                        if(selectionData.objsSelected.contains(parent)) {
                            wrongMovement = true;
                            break;
                        }
                    }
                    if(!wrongMovement) {
                        displayData.moveObjectsToListAtIndex(hoveredObjValToUse.parents.back(), indexToMoveTo, selectionData.orderedObjsSelected);
                        selectionData.objsSelected.clear();
                        selectionData.orderedObjsSelected.clear();
                    }
                    else
                        set_single_selected_object(selectionData);
                }
            }
            objDragged = std::nullopt;
        }
    }

    oldHoveredObject = hoveredObject;
}

void TreeListing::set_single_selected_object(SelectionData& selectionData) {
    selectionData.objsSelected.clear();
    selectionData.orderedObjsSelected.clear();
    selectionData.objsSelected.emplace(objDragged.value().object);
    selectionData.orderedObjsSelected.emplace_back(objDragged.value());
}

void TreeListing::recursive_gui(UpdateInputData& io, GUIManager& gui, const DisplayData& displayData, SelectionData& selectionData, NetworkingObjects::NetObjID objID, size_t& itemCount, size_t itemCountToStartAt, size_t itemCountToEndAt, size_t listDepth, std::optional<ParentObjectIDStack>& hoveredObject, std::vector<NetworkingObjects::NetObjID>& parentIDStack, bool parentJustSelected) {
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

                bool isOldHoveredObject = oldHoveredObject.has_value() && oldHoveredObject.value().object == objInList.id;
                SkColor4f backgroundColor;
                if(selectionData.objsSelected.contains(objInList.id))
                    backgroundColor = io.theme->backColor1;
                else if(isOldHoveredObject && objInList.isDirectory && !objInList.isDirectoryOpen && objDragged.has_value())
                    backgroundColor = lerp_vec(io.theme->backColor2, io.theme->fillColor1, std::clamp(std::chrono::duration<float, std::chrono::milliseconds::period>(std::chrono::steady_clock::now() - timeStartedHoveringOverObject) / TIME_TO_HOVER_TO_OPEN_DIRECTORY, 0.0f, 1.0f));
                else
                    backgroundColor = io.theme->backColor2;

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
                    .backgroundColor = convert_vec4<Clay_Color>(backgroundColor)
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
                        if(!isOldHoveredObject)
                            timeStartedHoveringOverObject = std::chrono::steady_clock::now();
                        if(io.mouse.leftClick && !buttonClicked && !parentJustSelected) {
                            if(io.key.leftShift)
                                dragHoldSelected = true;
                            else if(io.key.leftCtrl) {
                                bool alreadySelected = selectionData.objsSelected.contains(objInList.id);
                                if(alreadySelected)
                                    selectionData.objsSelected.erase(objInList.id);
                                else {
                                    select_and_unselect_parents(parentIDStack, objInList.id, selectionData);
                                    selfJustSelected = true;
                                }
                            }
                            else {
                                if(!selectionData.objsSelected.contains(objInList.id)) {
                                    selectionData.objsSelected.clear();
                                    selectionData.orderedObjsSelected.clear();
                                    selectionData.objsSelected.emplace(objInList.id);
                                    selfJustSelected = true;
                                }
                                objDragged = {objID, objInList.id};
                            }
                        }
                        if(io.key.leftShift && dragHoldSelected && !parentJustSelected) {
                            select_and_unselect_parents(parentIDStack, objInList.id, selectionData);
                            selfJustSelected = true;
                        }
                    }
                }
                gui.pop_id();
            }
            if(!parentJustSelected) {
                if(selectionData.objsSelected.contains(objInList.id))
                    selectionData.orderedObjsSelected.emplace_back(objID, objInList.id);
            }
            else
                unselect_object(objInList.id, selectionData);
            itemCount++;
            if(objInList.isDirectoryOpen)
                recursive_gui(io, gui, displayData, selectionData, objInList.id, itemCount, itemCountToStartAt, itemCountToEndAt, listDepth + 1, hoveredObject, parentIDStack, selfJustSelected | parentJustSelected);
        }
    }
    parentIDStack.pop_back();
}

void TreeListing::unselect_object(NetworkingObjects::NetObjID id, SelectionData& selectionData) {
    selectionData.objsSelected.erase(id);
    std::erase_if(selectionData.orderedObjsSelected, [&id](const auto& p) {
        return p.object == id;
    });
}

void TreeListing::select_and_unselect_parents(const std::vector<NetworkingObjects::NetObjID>& parentIDStack, NetworkingObjects::NetObjID newSelectedObj, SelectionData& selectionData) {
    for(const NetworkingObjects::NetObjID& parent : parentIDStack) {
        if(selectionData.objsSelected.contains(parent)) {
            unselect_object(parent, selectionData);
            break;
        }
    }
    selectionData.objsSelected.emplace(newSelectedObj);
}

void TreeListing::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) {
}

}
