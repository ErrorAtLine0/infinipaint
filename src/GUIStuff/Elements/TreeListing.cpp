#include "TreeListing.hpp"
#include "Helpers/ConvertVec.hpp"
#include "ScrollArea.hpp"
#include "../GUIManager.hpp"

#include "../ElementHelpers/ScrollAreaHelpers.hpp"
#include "../ElementHelpers/ButtonHelpers.hpp"
#include "LayoutElement.hpp"

#define TIME_TO_HOVER_TO_OPEN_DIRECTORY std::chrono::milliseconds(700)

bool operator<(const GUIStuff::TreeListingObjIndexList& a, const GUIStuff::TreeListingObjIndexList& b) {
    size_t biggestIndexSize = std::max(a.size(), b.size());
    for(size_t i = 0; i < biggestIndexSize; i++) {
        if(i == a.size())
            return true;
        else if(i == b.size())
            return false;
        else if(a[i] < b[i])
            return true;
        else if(a[i] > b[i])
            return false;
    }
    return false;
}

namespace GUIStuff {

bool is_tree_listing_obj_index_parent(const TreeListingObjIndexList& parentToCheck, const TreeListingObjIndexList& obj) {
    if(parentToCheck.size() >= obj.size())
        return false;
    for(size_t i = 0; i < parentToCheck.size(); i++) {
        if(parentToCheck[i] != obj[i])
            return false;
    }
    return true;
}

using namespace ElementHelpers;

TreeListing::TreeListing(GUIManager& gui):
    Element(gui) {}

void TreeListing::layout(const Clay_ElementId& id, const Data& newDisplayData) {
    d = newDisplayData;

    flattenedIndexList.clear();
    recursive_visible_flattened_obj_list(flattenedIndexList, {});

    CLAY(id, {
        .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}
    }) {
        gui.element<LayoutElement>("scroll parent", [&] (LayoutElement*, const Clay_ElementId& lIdParent) {
            CLAY(lIdParent, {
                .layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}
            }) {
                scroll_area_many_entries(gui, "scroll", {
                    .entryHeight = ENTRY_HEIGHT,
                    .entryCount = flattenedIndexList.size(),
                    .elementContent = [&](size_t i) {
                        const ObjInfo& objInfo = flattenedIndexList[i];
                        gui.element<LayoutElement>("elem", [&](LayoutElement*, const Clay_ElementId& lId) {
                            SkColor4f backgroundColor;
                            if(d.selectedIndices && d.selectedIndices->contains(flattenedIndexList[i].objIndex))
                                backgroundColor = gui.io.theme->backColor1;
                            else
                                backgroundColor = gui.io.theme->backColor2;

                            CLAY(lId, {
                                .layout = {
                                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                                    .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                                },
                                .backgroundColor = convert_vec4<Clay_Color>(backgroundColor)
                            }) {
                                if(objInfo.objIndex.size() > 1) {
                                    CLAY_AUTO_ID({
                                        .layout = {.sizing = {.width = CLAY_SIZING_FIXED((objInfo.objIndex.size() - 1) * ICON_SIZE), .height = CLAY_SIZING_GROW(0)}}
                                    }) {}
                                }

                                uint16_t topBorderWidth = (isDragging && dragFromTop && i == dragIndexEnd && dragIndexStart != dragIndexEnd) ? 1 : 0;
                                uint16_t bottomBorderWidth = (isDragging && !dragFromTop && i == dragIndexEnd && dragIndexStart != dragIndexEnd) ? 1 : 0;
                                CLAY_AUTO_ID({
                                    .layout = {
                                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                                        .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                                    },
                                    .border = {
                                        .color = convert_vec4<Clay_Color>(gui.io.theme->frontColor1),
                                        .width = {
                                            .left = 0,
                                            .right = 0,
                                            .top = topBorderWidth,
                                            .bottom = bottomBorderWidth,
                                            .betweenChildren = 0
                                        }
                                    }
                                }) {
                                    CLAY_AUTO_ID({
                                        .layout = {.sizing = {.width = CLAY_SIZING_FIXED(ICON_SIZE), .height = CLAY_SIZING_FIXED(ICON_SIZE)}}
                                    }) {
                                        if(!objInfo.isDirectory)
                                            d.drawNonDirectoryObjIconGUI(objInfo.objIndex);
                                        else {
                                            gui.set_z_index_keep_clipping_region(gui.get_z_index() + 1, [&] {
                                                svg_icon_button(gui, "open dir button", objInfo.isOpen ? "data/icons/droparrow.svg" : "data/icons/droparrowclose.svg", {
                                                    .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                                                    .size = ICON_SIZE,
                                                    .onClick = [&] {
                                                        d.setDirectoryOpen(objInfo.objIndex, !objInfo.isOpen);
                                                        if(d.selectedIndices) {
                                                            d.selectedIndices->clear();
                                                            if(d.onSelectChange) d.onSelectChange();
                                                        }
                                                    }
                                                });
                                            });
                                        }
                                    }
                                    d.drawObjGUI(objInfo.objIndex);
                                }
                            }
                        }, LayoutElement::Callbacks {
                            .mouseButton = [&, i] (LayoutElement* l, const InputManager::MouseButtonCallbackArgs& button) {
                                if(l->mouseHovering && button.button == InputManager::MouseButton::LEFT && button.down) {
                                    gui.set_post_callback_func([&, i, button, l]{
                                        auto& objIndex = flattenedIndexList[i].objIndex;
                                        if(d.selectedIndices) {
                                            auto it = d.selectedIndices->find(objIndex);
                                            bool wasSelected = it != d.selectedIndices->end();

                                            if(gui.io.input->key(InputManager::KEY_GENERIC_LSHIFT).held) {
                                                if(!wasSelected) {
                                                    d.selectedIndices->emplace(objIndex);
                                                    if(d.onSelectChange) d.onSelectChange();
                                                }
                                            }
                                            else if(gui.io.input->key(InputManager::KEY_GENERIC_LCTRL).held) {
                                                if(!wasSelected)
                                                    d.selectedIndices->emplace(objIndex);
                                                else
                                                    d.selectedIndices->erase(it);
                                                if(d.onSelectChange) d.onSelectChange();
                                            }
                                            else if(button.clicks == 2) {
                                                if(d.onDoubleClick) d.onDoubleClick(objIndex);
                                            }
                                            else {
                                                if(button.clicks == 2 && d.onDoubleClick) d.onDoubleClick(objIndex);
                                                if(!wasSelected) {
                                                    d.selectedIndices->clear();
                                                    d.selectedIndices->emplace(objIndex);
                                                    if(d.onSelectChange) d.onSelectChange();
                                                }
                                                isDragging = true;
                                                dragIndexStart = dragIndexEnd = i;
                                                dragFromTop = button.pos.y() - l->get_bb().value().min.y() < ENTRY_HEIGHT * 0.5f;
                                            }
                                        }
                                        else if(button.clicks == 2)
                                            if(d.onDoubleClick) d.onDoubleClick(objIndex);
                                    });
                                    gui.set_to_layout();
                                }
                            },
                            .mouseMotion = [&, i] (LayoutElement* l, const InputManager::MouseMotionCallbackArgs& motion) {
                                if(isDragging && l->mouseHovering) {
                                    size_t newDragIndexEnd = i;
                                    bool newDragFromTop = motion.pos.y() - l->get_bb().value().min.y() < ENTRY_HEIGHT * 0.5f;
                                    if(dragIndexEnd != newDragIndexEnd || dragFromTop != newDragFromTop) {
                                        dragIndexEnd = newDragIndexEnd;
                                        dragFromTop = newDragFromTop;
                                        gui.set_to_layout();
                                    }
                                }
                            }
                        });
                    }
                });
            }
        }, LayoutElement::Callbacks {
            .mouseButton = [&] (LayoutElement* l, const InputManager::MouseButtonCallbackArgs& button) {
                if(button.button == InputManager::MouseButton::LEFT && !button.down) {
                    if(isDragging) {
                        gui.set_to_layout();
                        isDragging = false;
                        if(l->mouseHovering || l->childMouseHovering) {
                            if(dragIndexStart != dragIndexEnd)
                                gui.set_post_callback_func([&] {move_selected_objects();});
                            else if(!gui.io.input->key(InputManager::KEY_GENERIC_LSHIFT).held && !gui.io.input->key(InputManager::KEY_GENERIC_LCTRL).held) {
                                gui.set_post_callback_func([&, i = dragIndexEnd] {
                                    if(d.selectedIndices->size() != 1 || !d.selectedIndices->contains(flattenedIndexList[i].objIndex)) {
                                        d.selectedIndices->clear();
                                        d.selectedIndices->emplace(flattenedIndexList[i].objIndex);
                                        if(d.onSelectChange) d.onSelectChange();
                                    }
                                });
                            }
                        }
                    }
                }
            }
        });
    }
}

void TreeListing::move_selected_objects() {
    if(d.selectedIndices && dragIndexEnd < flattenedIndexList.size()) {
        TreeListingObjIndexList objIndexToMoveTo = flattenedIndexList[dragIndexEnd].objIndex;
        if(!dragFromTop) {
            if(flattenedIndexList[dragIndexEnd].isOpen) // Drag into open directory
                objIndexToMoveTo.emplace_back(0);
            else {
                objIndexToMoveTo.back() += 1;
                if(dragIndexStart < flattenedIndexList.size()) {
                    if(objIndexToMoveTo == flattenedIndexList[dragIndexStart].objIndex)
                        objIndexToMoveTo.back() += 1;
                }
            }
        }

        // Check if object is about to move into itself (wrong movement)
        bool wrongMovement = false;
        for(const TreeListingObjIndexList& o : *d.selectedIndices) {
            if(is_tree_listing_obj_index_parent(o, objIndexToMoveTo)) {
                wrongMovement = true;
                break;
            }
        }
        if(!wrongMovement) {
            std::vector<TreeListingObjIndexList> toMove(d.selectedIndices->begin(), d.selectedIndices->end());
            d.selectedIndices->clear();
            if(d.onSelectChange) d.onSelectChange();
            if(d.moveObj) d.moveObj(toMove, objIndexToMoveTo);
        }
    }
}

void TreeListing::recursive_visible_flattened_obj_list(std::vector<ObjInfo>& objs, const TreeListingObjIndexList& objIndex) {
    std::optional<DirectoryInfo> dirInfo = d.dirInfo(objIndex);
    bool isRoot = objIndex.empty();
    if(!isRoot)
        objs.emplace_back(objIndex, dirInfo.has_value(), dirInfo.has_value() ? dirInfo.value().isOpen : false);
    if(dirInfo.has_value() && (dirInfo.value().isOpen || isRoot)) {
        TreeListingObjIndexList childObjIndex = objIndex;
        childObjIndex.emplace_back();
        for(size_t i = 0; i < dirInfo.value().dirSize; i++) {
            childObjIndex.back() = i;
            recursive_visible_flattened_obj_list(objs, childObjIndex);
        }
    }
}

}
