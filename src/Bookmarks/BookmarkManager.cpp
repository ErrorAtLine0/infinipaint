#include "BookmarkManager.hpp"
#include "../World.hpp"
#include "../MainProgram.hpp"
#include "BookmarkListItem.hpp"
#include "Helpers/NetworkingObjects/NetObjOrderedList.hpp"
#include "Helpers/NetworkingObjects/NetObjTemporaryPtr.decl.hpp"
#include <cereal/types/unordered_map.hpp>

BookmarkManager::BookmarkManager(World& w):
    world(w)
{}

void BookmarkManager::server_init_no_file() {
    bookmarkListRoot = world.netObjMan.make_obj_from_ptr<BookmarkListItem>(new BookmarkListItem(world.netObjMan, "ROOT", true, {}));
}

void BookmarkManager::refresh_gui_data() {
    selectionData = GUIStuff::TreeListing::SelectionData();
    oldSelection.clear();
    nameToEdit.clear();
    nameForNew.clear();
}

void BookmarkManager::setup_list_gui(const std::string& id) {
    using namespace NetworkingObjects;
    if(bookmarkListRoot) {
        auto& gui = world.main.toolbar.gui;
        gui.push_id(id);
        if(bookmarkListRoot->get_folder_list()->empty()) {
            gui.text_label_centered("No bookmarks yet...");
            selectionData = GUIStuff::TreeListing::SelectionData();
            oldSelection.clear();
            nameToEdit.clear();
        }
        else {
            std::optional<NetObjID> toDeleteParent;
            std::optional<NetObjID> toDeleteObject;
            gui.tree_listing("list", bookmarkListRoot.get_net_id(), GUIStuff::TreeListing::DisplayData{
                .getObjInListAtIndex = [&](NetObjID parentId, size_t index) -> std::optional<GUIStuff::TreeListing::DisplayData::ObjInList> {
                    NetObjOrderedList<BookmarkListItem>& bookmarkParentFolder = *world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(parentId)->get_folder_list();
                    if(bookmarkParentFolder.size() <= index)
                        return std::nullopt;
                    auto itemAtIndex = bookmarkParentFolder.at(index);
                    return GUIStuff::TreeListing::DisplayData::ObjInList{
                        .id = itemAtIndex->obj.get_net_id(),
                        .isDirectory = itemAtIndex->obj->is_folder(),
                        .isDirectoryOpen = itemAtIndex->obj->is_folder() ? itemAtIndex->obj->is_folder_open() : false
                    };
                },
                .getIndexOfObjInList = [&](const GUIStuff::TreeListing::ParentObjectIDPair& idPair) {
                    return world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(idPair.parent)->get_folder_list()->get(idPair.object)->pos;
                },
                .setDirectoryOpen = [&](NetObjID netID, bool newDirectoryOpen) {
                    world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(netID)->set_folder_open(newDirectoryOpen);
                },
                .drawNonDirectoryObjIconGUI = [&](NetObjID netID) {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            .padding = CLAY_PADDING_ALL(2),
                        },
                    }) {
                        gui.svg_icon("bookmark ico", "data/icons/bookmark.svg");
                    }
                    return false;
                },
                .drawObjGUI = [&](const GUIStuff::TreeListing::ParentObjectIDPair& idPair) {
                    auto tempPtr = world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(idPair.object);
                    bool isButtonClicked = false;
                    if(Clay_Hovered() && gui.io->mouse.leftClick >= 2 && !tempPtr->is_folder() && selectionData.objsSelected.contains(idPair.object) && !gui.io->key.leftCtrl && !gui.io->key.leftShift)
                        tempPtr->get_bookmark_data().jump_to(world);
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                        },
                    }) {
                        gui.text_label(tempPtr->get_name());
                    }
                    if(!tempPtr->is_folder() && gui.svg_icon_button_transparent("jump button", "data/icons/jump.svg", false, GUIStuff::TreeListing::ENTRY_HEIGHT)) {
                        tempPtr->get_bookmark_data().jump_to(world);
                        isButtonClicked = true;
                    }
                    if(gui.svg_icon_button_transparent("delete button", "data/icons/trash.svg", false, GUIStuff::TreeListing::ENTRY_HEIGHT)) {
                        toDeleteParent = idPair.parent;
                        toDeleteObject = idPair.object;
                        isButtonClicked = true;
                    }
                    return isButtonClicked;
                },
                .moveObjectsToListAtIndex = [&](NetObjID listObj, size_t index, const std::vector<GUIStuff::TreeListing::ParentObjectIDPair>& objsToInsert) {
                    world.netObjMan.send_multi_update_messsage([&]() {
                        std::unordered_map<NetObjID, std::vector<NetObjOrderedListIterator<BookmarkListItem>>> toEraseMap;
                        uint32_t newIndex = index;
                        std::vector<NetObjOwnerPtr<BookmarkListItem>> toInsertObjPtrs;
                        auto& listPtr = world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(listObj)->get_folder_list();

                        for(size_t i = 0; i < objsToInsert.size(); i++) {
                            auto& parentListPtr = world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(objsToInsert[i].parent)->get_folder_list();
                            toEraseMap[objsToInsert[i].parent].emplace_back(parentListPtr->get(objsToInsert[i].object));
                            if(newIndex != 0 && objsToInsert[i].parent == listObj && listPtr->get(objsToInsert[i].object)->pos <= index)
                                newIndex--;
                        }

                        for(auto& [listToEraseFrom, setToErase] : toEraseMap) {
                            auto& listToEraseFromPtr = world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(listToEraseFrom)->get_folder_list();
                            listToEraseFromPtr->erase_list(listToEraseFromPtr, setToErase, &toInsertObjPtrs);
                        }

                        std::vector<std::pair<NetObjOrderedListIterator<BookmarkListItem>, NetObjOwnerPtr<BookmarkListItem>>> toInsert;
                        auto insertIt = listPtr->at(newIndex);
                        for(uint32_t i = 0; i < objsToInsert.size(); i++) {
                            auto it = std::find_if(toInsertObjPtrs.begin(), toInsertObjPtrs.end(), [id = objsToInsert[i].object](NetObjOwnerPtr<BookmarkListItem>& objPtr) {
                                return objPtr.get_net_id() == id;
                            });
                            toInsert.emplace_back(insertIt, std::move(*it));
                            toInsert.back().second.reassign_ids();
                        }
                        listPtr->insert_ordered_list_and_send_create(listPtr, toInsert);
                    }, NetObjManager::SendUpdateType::SEND_TO_ALL, nullptr);
                }
            }, selectionData);


            if(selectionData.objsSelected != oldSelection) {
                oldSelection = selectionData.objsSelected;
                if(selectionData.objsSelected.size() == 1) {
                    NetObjTemporaryPtr<BookmarkListItem> tempPtr = world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(*selectionData.objsSelected.begin());
                    if(tempPtr)
                        nameToEdit = tempPtr->get_name();
                    else {
                        nameToEdit.clear();
                    }
                }
                else
                    nameToEdit.clear();
            }

            if(toDeleteObject.has_value()) {
                auto& folderList = world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(toDeleteParent.value())->get_folder_list();
                folderList->erase(folderList, folderList->get(toDeleteObject.value()));
            }
        }

        gui.left_to_right_line_layout([&]() {
            bool addByEnter = false;
            gui.input_text("input new name", &nameForNew, true, [&](GUIStuff::SelectionHelper& s) {
                addByEnter = s.selected && gui.io->key.enter;
            });
            
            if(gui.svg_icon_button("create new bookmark", "data/icons/plusbold.svg", false, gui.SMALL_BUTTON_SIZE) || addByEnter)
                create_in_proper_position(new BookmarkListItem(world.netObjMan, nameForNew, false, {world.drawData.cam.c, world.main.window.size.cast<int32_t>()}));
            else if(gui.svg_icon_button("create new folder", "data/icons/folderbold.svg", false, gui.SMALL_BUTTON_SIZE)) { 
                auto newFolderObjInfo = create_in_proper_position(new BookmarkListItem(world.netObjMan, nameForNew, true, {}));
                newFolderObjInfo->obj->set_folder_open(true);
            }
        });

        if(selectionData.objsSelected.size() == 1) {
            NetObjTemporaryPtr<BookmarkListItem> tempPtr = world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(*selectionData.objsSelected.begin());
            if(tempPtr) {
                gui.text_label_centered(tempPtr->is_folder() ? "Edit Bookmark Folder" : "Edit Bookmark");
                gui.input_text_field("input edit name", "Name", &nameToEdit);
                tempPtr->set_name(world.delayedUpdateObjectManager, nameToEdit);
            }
        }

        gui.pop_id();
    }
}

std::optional<NetworkingObjects::NetObjOrderedListIterator<BookmarkListItem>> BookmarkManager::try_to_create_in_proper_position(BookmarkListItem* newItem) {
    using namespace NetworkingObjects;
    if(selectionData.orderedObjsSelected.empty())
        return std::nullopt;
    auto& backObjParentObjectPair = selectionData.orderedObjsSelected.back();
    NetObjTemporaryPtr<BookmarkListItem> parentPtr = world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(backObjParentObjectPair.parent);
    if(!parentPtr)
        return std::nullopt;
    NetObjTemporaryPtr<BookmarkListItem> objectPtr = world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(backObjParentObjectPair.object);
    if(!objectPtr)
        return std::nullopt;
    if(objectPtr->is_folder()) {
        objectPtr->set_folder_open(true);
        return objectPtr->get_folder_list()->push_back_and_send_create(objectPtr->get_folder_list(), newItem);
    }
    return parentPtr->get_folder_list()->insert_and_send_create(parentPtr->get_folder_list(), std::next(parentPtr->get_folder_list()->get(objectPtr.get_net_id())), newItem);
}

NetworkingObjects::NetObjOrderedListIterator<BookmarkListItem> BookmarkManager::create_in_proper_position(BookmarkListItem* newItem) {
    auto toRet = try_to_create_in_proper_position(newItem);
    if(!toRet)
        return bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), newItem);
    return toRet.value();
}

void BookmarkManager::scale_up(const WorldScalar& scaleUpAmount) {
    bookmarkListRoot->scale_up(scaleUpAmount);
}

void BookmarkManager::save_file(cereal::PortableBinaryOutputArchive& a) const {
    bookmarkListRoot->save_file(a);
}

void BookmarkManager::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    if(version >= VersionNumber(0, 4, 0)) {
        bookmarkListRoot = world.netObjMan.make_obj_from_ptr<BookmarkListItem>(new BookmarkListItem());
        bookmarkListRoot->load_file(a, version, *this);
    }
    else {
        server_init_no_file();
        std::unordered_map<std::string, BookmarkData> bookmarkMap;
        std::vector<std::pair<std::string, BookmarkData>> bookmarkDataSorted;
        a(bookmarkMap);
        for(auto& [name, bData] : bookmarkMap)
            bookmarkDataSorted.emplace_back(name, bData);
        std::sort(bookmarkDataSorted.begin(), bookmarkDataSorted.end(), [](auto& aPair, auto& bPair) {
            return aPair.first < bPair.first;
        });
        for(auto& [name, bData] : bookmarkDataSorted)
            bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, name, false, bData));
    }
}
