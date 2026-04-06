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
                }, LayoutElement::Callbacks {
                    .mouseButton = [&, i] (LayoutElement* l, const InputManager::MouseButtonCallbackArgs& button) {
                        if(l->mouseHovering && button.button == InputManager::MouseButton::LEFT && button.down) {
                            gui.set_post_callback_func([&]{
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
                                    else if(!wasSelected || d.selectedIndices->size() != 1) {
                                        d.selectedIndices->clear();
                                        d.selectedIndices->emplace(objIndex);
                                        if(d.onSelectChange) d.onSelectChange();
                                    }
                                    else if(button.clicks == 2)
                                        if(d.onDoubleClick) d.onDoubleClick(objIndex);
                                }
                                else if(button.clicks == 2)
                                    if(d.onDoubleClick) d.onDoubleClick(objIndex);
                            });
                            gui.set_to_layout();
                        }
                    }
                });
            }
        });
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
