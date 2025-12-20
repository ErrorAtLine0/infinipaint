#include "TreeListing.hpp"
#include "../GUIManager.hpp"
#include "Helpers/ConvertVec.hpp"

constexpr float ENTRY_HEIGHT = 25.0f;

namespace GUIStuff {

void TreeListing::update(UpdateInputData& io, GUIManager& gui, const NetworkingObjects::NetObjTemporaryPtr<NetworkingObjects::NetObjOrderedList<BookmarkListItem>>& bookmarkList) {
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
            recursive_gui(io, gui, bookmarkList, itemCount, itemCountToStartAt, itemCountToEndAt, 0);
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

void TreeListing::recursive_gui(UpdateInputData& io, GUIManager& gui, const NetworkingObjects::NetObjTemporaryPtr<NetworkingObjects::NetObjOrderedList<BookmarkListItem>>& bookmarkList, size_t& itemCount, size_t itemCountToStartAt, size_t itemCountToEndAt, size_t listDepth) {
    std::optional<NetworkingObjects::NetObjID> toDelete;
    for(auto& bookmarkItem : bookmarkList->get_data()) {
        if(itemCount >= itemCountToStartAt && itemCount < itemCountToEndAt) {
            gui.push_id(itemCount);
            bool buttonClicked = false;
            CLAY_AUTO_ID({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(ENTRY_HEIGHT)},
                    .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                },
                .backgroundColor = convert_vec4<Clay_Color>((objSelected.has_value() && objSelected.value() == bookmarkItem->obj.get_net_id()) ? io.theme->backColor1 : io.theme->backColor2),
            }) {
                if(listDepth != 0) {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_FIXED(ENTRY_HEIGHT * listDepth), .height = CLAY_SIZING_FIXED(ENTRY_HEIGHT)}
                        },
                    }) {}
                }
                if(bookmarkItem->obj->is_folder()) {
                    if(gui.svg_icon_button_transparent("ico", bookmarkItem->obj->is_folder_open() ? "data/icons/droparrow.svg" : "data/icons/droparrowclose.svg", false, ENTRY_HEIGHT)) {
                        bookmarkItem->obj->set_folder_open(!bookmarkItem->obj->is_folder_open());
                        buttonClicked = true;
                    }
                }
                else {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_FIXED(ENTRY_HEIGHT), .height = CLAY_SIZING_FIXED(ENTRY_HEIGHT)},
                            .padding = CLAY_PADDING_ALL(2)
                        }
                    }) {
                        gui.svg_icon("ico", "data/icons/bookmark.svg");
                    }
                }
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                        .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                    }
                }) {
                    gui.text_label(bookmarkItem->obj->get_name());
                }
                if(gui.svg_icon_button_transparent("delete", "data/icons/trash.svg", false, ENTRY_HEIGHT)) {
                    toDelete = bookmarkItem->obj.get_net_id();
                    buttonClicked = true;
                }
                if(Clay_Hovered() && io.mouse.leftClick && !buttonClicked)
                    objSelected = bookmarkItem->obj.get_net_id();
            }
            gui.pop_id();
        }

        itemCount++;
        if(bookmarkItem->obj->is_folder() && bookmarkItem->obj->is_folder_open())
            recursive_gui(io, gui, bookmarkItem->obj->get_folder_list(), itemCount, itemCountToStartAt, itemCountToEndAt, listDepth + 1);
    }
    if(toDelete.has_value())
        bookmarkList->erase(bookmarkList, toDelete.value());
}

void TreeListing::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) {
}

}
