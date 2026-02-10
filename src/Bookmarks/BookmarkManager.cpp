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
    editing_bookmark_check();
    oldSelection.clear();
    editingBookmarkName = std::nullopt;
    nameToEdit.clear();
    nameForNew.clear();
}

void BookmarkManager::setup_list_gui(const char* id) {
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
                    std::unordered_map<WorldUndoManager::UndoObjectID, std::pair<std::vector<uint32_t>, std::vector<WorldUndoManager::UndoObjectID>>> toEraseMapUndo;
                    WorldUndoManager::UndoObjectID insertParentUndoID;
                    uint32_t insertUndoPosition;
                    std::vector<WorldUndoManager::UndoObjectID> insertedUndoIDs;

                    world.netObjMan.send_multi_update_messsage([&]() {
                        std::unordered_map<NetObjID, std::vector<NetObjOrderedListIterator<BookmarkListItem>>> toEraseMap;
                        uint32_t newIndex = index;
                        std::vector<NetObjOwnerPtr<BookmarkListItem>> toInsertObjPtrs;
                        auto& listPtr = world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(listObj)->get_folder_list();

                        // Map parent folders to list of objects that should be erased from them
                        for(size_t i = 0; i < objsToInsert.size(); i++) {
                            auto& parentListPtr = world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(objsToInsert[i].parent)->get_folder_list();
                            toEraseMap[objsToInsert[i].parent].emplace_back(parentListPtr->get(objsToInsert[i].object));
                            if(newIndex != 0 && objsToInsert[i].parent == listObj && listPtr->get(objsToInsert[i].object)->pos <= index)
                                newIndex--;
                        }

                        // Prepare erase map for undo
                        for(auto& [parentNetID, objsToEraseList] : toEraseMap) {
                            auto& undoEraseListPairForParent = toEraseMapUndo[world.undo.get_undoid_from_netid(parentNetID)];
                            uint32_t i = 0;
                            for(auto& objToErase : objsToEraseList) {
                                undoEraseListPairForParent.first.emplace_back(objToErase->pos - i);
                                undoEraseListPairForParent.second.emplace_back(world.undo.get_undoid_from_netid(objToErase->obj.get_net_id()));
                                i++;
                            }
                        }

                        // Erase the list of objects we previously mapped to each parent, and move the erased objects to vector toInsertObjPtrs
                        for(auto& [listToEraseFrom, setToErase] : toEraseMap) {
                            auto& listToEraseFromPtr = world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(listToEraseFrom)->get_folder_list();
                            listToEraseFromPtr->erase_list(listToEraseFromPtr, setToErase, &toInsertObjPtrs);
                        }

                        // Insert objects in toInsertObjPtrs into the folder we have selected
                        std::vector<std::pair<NetObjOrderedListIterator<BookmarkListItem>, NetObjOwnerPtr<BookmarkListItem>>> toInsert;
                        auto insertIt = listPtr->at(newIndex);
                        for(uint32_t i = 0; i < objsToInsert.size(); i++) {
                            auto it = std::find_if(toInsertObjPtrs.begin(), toInsertObjPtrs.end(), [id = objsToInsert[i].object](NetObjOwnerPtr<BookmarkListItem>& objPtr) {
                                return objPtr.get_net_id() == id;
                            });
                            toInsert.emplace_back(insertIt, std::move(*it));
                            toInsert.back().second.reassign_ids();
                        }
                        std::vector<NetObjOrderedListIterator<BookmarkListItem>> insertedIterators = listPtr->insert_sorted_list_and_send_create(listPtr, toInsert);

                        // Prepare insert list for undo
                        insertParentUndoID = world.undo.get_undoid_from_netid(listObj);
                        insertUndoPosition = newIndex;
                        for(auto& it : insertedIterators)
                            insertedUndoIDs.emplace_back(world.undo.get_undoid_from_netid(it->obj.get_net_id()));
                    }, NetObjManager::SendUpdateType::SEND_TO_ALL, nullptr);

                    class MoveBookmarksWorldUndoAction : public WorldUndoAction {
                        public:
                            // If the undo ID maps to the net id, we can also assume that the object is definitely inside the list we want to move it from, and not inside some other list
                            // If it was moved to another list, there would be an undo move that would move it back to this list, and if it was moved by another player the undo id wouldnt map back to the net id, and the undo would fail
                            // We can also assume all objects in all lists remain in the same order for the same reason, so no need to sort anything if it was sorted when the undo action was generated
                            MoveBookmarksWorldUndoAction(const std::unordered_map<WorldUndoManager::UndoObjectID, std::pair<std::vector<uint32_t>, std::vector<WorldUndoManager::UndoObjectID>>>& initUndoEraseMap,
                                                         WorldUndoManager::UndoObjectID initInsertParentUndoID,
                                                         uint32_t initInsertPosition,
                                                         std::vector<WorldUndoManager::UndoObjectID> initInsertedUndoIDs):
                            undoEraseMap(initUndoEraseMap),
                            insertParentUndoID(initInsertParentUndoID),
                            insertPosition(initInsertPosition),
                            insertedUndoIDs(initInsertedUndoIDs)
                            {}
                            std::string get_name() const override {
                                return "Move Bookmarks";
                            }
                            bool undo(WorldUndoManager& undoMan) override {
                                std::vector<NetObjOwnerPtr<BookmarkListItem>> toMoveObjs;
                                std::vector<NetObjOrderedListIterator<BookmarkListItem>> toEraseList;
                                NetObjTemporaryPtr<NetObjOrderedList<BookmarkListItem>> eraseListPtr;
                                std::unordered_map<NetObjID, std::pair<std::vector<uint32_t>*, std::vector<NetObjID>>> toInsertToMap;

                                {
                                    std::optional<NetObjID> eraseListNetID = undoMan.get_netid_from_undoid(insertParentUndoID);
                                    if(!eraseListNetID.has_value())
                                        return false;
                                    eraseListPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(eraseListNetID.value())->get_folder_list();
                                }

                                {
                                    std::vector<NetObjID> toEraseNetIDList;
                                    if(!undoMan.fill_netid_list_from_undoid_list(toEraseNetIDList, insertedUndoIDs))
                                        return false;
                                    toEraseList = eraseListPtr->get_list(toEraseNetIDList);
                                }

                                {
                                    for(auto& [undoID, indexUndoIdListPair] : undoEraseMap) {
                                        std::optional<NetObjID> netID = undoMan.get_netid_from_undoid(undoID);
                                        if(!netID.has_value())
                                            return false;
                                        auto& insertMapEntry = toInsertToMap[netID.value()];
                                        insertMapEntry.first = &indexUndoIdListPair.first;
                                        if(!undoMan.fill_netid_list_from_undoid_list(insertMapEntry.second, indexUndoIdListPair.second))
                                            return false;
                                    }
                                }

                                undoMan.world.netObjMan.send_multi_update_messsage([&]() {
                                    eraseListPtr->erase_list(eraseListPtr, toEraseList, &toMoveObjs);

                                    for(auto& [parentNetID, indexNetIDListPair] : toInsertToMap) {
                                        auto& parentListPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(parentNetID)->get_folder_list();
                                        std::vector<NetObjOrderedListIterator<BookmarkListItem>> insertIteratorList = parentListPtr->at_ordered_indices(*indexNetIDListPair.first);
                                        auto& insertNetIDList = indexNetIDListPair.second;
                                        std::vector<std::pair<NetObjOrderedListIterator<BookmarkListItem>, NetObjOwnerPtr<BookmarkListItem>>> toInsert;
                                        for(auto [insertIt, netID] : std::views::zip(insertIteratorList, insertNetIDList)) {
                                            auto it = std::find_if(toMoveObjs.begin(), toMoveObjs.end(), [id = netID](NetObjOwnerPtr<BookmarkListItem>& objPtr) {
                                                return objPtr.get_net_id() == id;
                                            });
                                            toInsert.emplace_back(insertIt, std::move(*it));
                                            toInsert.back().second.reassign_ids();
                                        }
                                        parentListPtr->insert_sorted_list_and_send_create(parentListPtr, toInsert);
                                    }
                                }, NetObjManager::SendUpdateType::SEND_TO_ALL, nullptr);
                                return true;
                            }
                            bool redo(WorldUndoManager& undoMan) override {
                                std::unordered_map<NetObjID, std::vector<NetObjID>> toEraseMap;
                                NetObjTemporaryPtr<NetObjOrderedList<BookmarkListItem>> listPtr;
                                std::vector<NetObjID> objsToInsert;

                                {
                                    for(auto& [undoID, indexUndoIDListPair] : undoEraseMap) {
                                        std::optional<NetObjID> netID = undoMan.get_netid_from_undoid(undoID);
                                        if(!netID.has_value())
                                            return false;
                                        if(!undoMan.fill_netid_list_from_undoid_list(toEraseMap[netID.value()], indexUndoIDListPair.second))
                                            return false;
                                    }
                                }

                                {
                                    std::optional<NetObjID> netID = undoMan.get_netid_from_undoid(insertParentUndoID);
                                    if(!netID.has_value())
                                        return false;
                                    listPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(netID.value())->get_folder_list();
                                }

                                {
                                    if(!undoMan.fill_netid_list_from_undoid_list(objsToInsert, insertedUndoIDs))
                                        return false;
                                }

                                undoMan.world.netObjMan.send_multi_update_messsage([&]() {
                                    std::vector<NetObjOwnerPtr<BookmarkListItem>> toInsertObjPtrs;

                                    for(auto& [listToEraseFrom, setToErase] : toEraseMap) {
                                        auto& listToEraseFromPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(listToEraseFrom)->get_folder_list();
                                        listToEraseFromPtr->erase_list(listToEraseFromPtr, listToEraseFromPtr->get_list(setToErase), &toInsertObjPtrs);
                                    }

                                    std::vector<std::pair<NetObjOrderedListIterator<BookmarkListItem>, NetObjOwnerPtr<BookmarkListItem>>> toInsert;
                                    auto insertIt = listPtr->at(insertPosition);
                                    for(uint32_t i = 0; i < objsToInsert.size(); i++) {
                                        auto it = std::find_if(toInsertObjPtrs.begin(), toInsertObjPtrs.end(), [id = objsToInsert[i]](NetObjOwnerPtr<BookmarkListItem>& objPtr) {
                                            return objPtr.get_net_id() == id;
                                        });
                                        toInsert.emplace_back(insertIt, std::move(*it));
                                        toInsert.back().second.reassign_ids();
                                    }
                                    listPtr->insert_sorted_list_and_send_create(listPtr, toInsert);
                                }, NetObjManager::SendUpdateType::SEND_TO_ALL, nullptr);

                                return true;
                            }
                            ~MoveBookmarksWorldUndoAction() {}

                            std::unordered_map<WorldUndoManager::UndoObjectID, std::pair<std::vector<uint32_t>, std::vector<WorldUndoManager::UndoObjectID>>> undoEraseMap;
                            WorldUndoManager::UndoObjectID insertParentUndoID;
                            uint32_t insertPosition;
                            std::vector<WorldUndoManager::UndoObjectID> insertedUndoIDs;
                    };
                    world.undo.push(std::make_unique<MoveBookmarksWorldUndoAction>(toEraseMapUndo, insertParentUndoID, insertUndoPosition, insertedUndoIDs));
                }
            }, selectionData);


            if(selectionData.objsSelected != oldSelection) {
                editing_bookmark_check();
                if(selectionData.objsSelected.size() == 1) {
                    NetObjTemporaryPtr<BookmarkListItem> tempPtr = world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(*selectionData.objsSelected.begin());
                    if(tempPtr)
                        nameToEdit = tempPtr->get_name();
                    else
                        nameToEdit.clear();
                }
                else
                    nameToEdit.clear();
            }

            if(toDeleteObject.has_value())
                remove_bookmark(toDeleteParent.value(), toDeleteObject.value());
        }

        gui.left_to_right_line_layout([&]() {
            bool addByEnter = false;
            gui.input_text("input new name", &nameForNew, true, [&](GUIStuff::SelectionHelper& s) {
                addByEnter = s.selected && gui.io->key.enter;
            });
            
            if(gui.svg_icon_button("create new bookmark", "data/icons/plusbold.svg", false, gui.SMALL_BUTTON_SIZE) || addByEnter)
                create_bookmark(new BookmarkListItem(world.netObjMan, nameForNew, false, {world.drawData.cam.c, world.main.window.size.cast<int32_t>()}));
            else if(gui.svg_icon_button("create new folder", "data/icons/folderbold.svg", false, gui.SMALL_BUTTON_SIZE)) { 
                auto newFolderObjInfo = create_bookmark(new BookmarkListItem(world.netObjMan, nameForNew, true, {}));
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

std::optional<std::pair<NetworkingObjects::NetObjID, NetworkingObjects::NetObjOrderedListIterator<BookmarkListItem>>> BookmarkManager::try_to_create_in_proper_position(BookmarkListItem* newItem) {
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
        return std::pair<NetworkingObjects::NetObjID, NetworkingObjects::NetObjOrderedListIterator<BookmarkListItem>>{objectPtr.get_net_id(), objectPtr->get_folder_list()->push_back_and_send_create(objectPtr->get_folder_list(), newItem)};
    }
    return std::pair<NetworkingObjects::NetObjID, NetworkingObjects::NetObjOrderedListIterator<BookmarkListItem>>{parentPtr.get_net_id(), parentPtr->get_folder_list()->insert_and_send_create(parentPtr->get_folder_list(), std::next(parentPtr->get_folder_list()->get(objectPtr.get_net_id())), newItem)};
}

std::pair<NetworkingObjects::NetObjID, NetworkingObjects::NetObjOrderedListIterator<BookmarkListItem>> BookmarkManager::create_in_proper_position(BookmarkListItem* newItem) {
    auto toRet = try_to_create_in_proper_position(newItem);
    if(!toRet)
        return {bookmarkListRoot.get_net_id(), bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), newItem)};
    return toRet.value();
}

NetworkingObjects::NetObjOrderedListIterator<BookmarkListItem> BookmarkManager::create_bookmark(BookmarkListItem* newItem) {
    class AddBookmarkWorldUndoAction : public WorldUndoAction {
        public:
            AddBookmarkWorldUndoAction(uint32_t newPos, WorldUndoManager::UndoObjectID initParentUndoID, WorldUndoManager::UndoObjectID initUndoID):
                pos(newPos),
                parentUndoID(initParentUndoID),
                undoID(initUndoID)
            {}
            std::string get_name() const override {
                return "Add Bookmark Item";
            }
            bool undo(WorldUndoManager& undoMan) override {
                std::optional<NetworkingObjects::NetObjID> toEraseParentID = undoMan.get_netid_from_undoid(parentUndoID);
                if(!toEraseParentID.has_value())
                    return false;
                std::optional<NetworkingObjects::NetObjID> toEraseID = undoMan.get_netid_from_undoid(undoID);
                if(!toEraseID.has_value())
                    return false;

                auto& bookmarkList = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(toEraseParentID.value())->get_folder_list();
                auto it = bookmarkList->get(toEraseID.value());
                bookmarkData = std::make_unique<BookmarkCompleteUndoData>(it->obj->get_complete_undo_data(undoMan));
                bookmarkList->erase(bookmarkList, it);
                return true;
            }
            bool redo(WorldUndoManager& undoMan) override {
                std::optional<NetworkingObjects::NetObjID> toInsertParentID = undoMan.get_netid_from_undoid(parentUndoID);
                if(!toInsertParentID.has_value())
                    return false;
                std::optional<NetworkingObjects::NetObjID> toInsertID = undoMan.get_netid_from_undoid(undoID);
                if(toInsertID.has_value())
                    return false;

                auto& bookmarkList = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(toInsertParentID.value())->get_folder_list();
                auto insertedIt = bookmarkList->emplace_direct(bookmarkList, bookmarkList->at(pos), undoMan.world, *bookmarkData);
                undoMan.register_new_netid_to_existing_undoid(undoID, insertedIt->obj.get_net_id());
                bookmarkData = nullptr;
                return true;
            }
            void scale_up(const WorldScalar& scaleAmount) override {
                if(bookmarkData)
                    bookmarkData->scale_up(scaleAmount);
            }
            ~AddBookmarkWorldUndoAction() {}

            std::unique_ptr<BookmarkCompleteUndoData> bookmarkData;
            uint32_t pos;
            WorldUndoManager::UndoObjectID parentUndoID;
            WorldUndoManager::UndoObjectID undoID;
    };

    auto insertedBookmarkPair = create_in_proper_position(newItem);
    auto& it = insertedBookmarkPair.second;
    world.undo.push(std::make_unique<AddBookmarkWorldUndoAction>(it->pos, world.undo.get_undoid_from_netid(insertedBookmarkPair.first), world.undo.get_undoid_from_netid(it->obj.get_net_id())));
    return it;
}

void BookmarkManager::remove_bookmark(const NetworkingObjects::NetObjID& parentID, const NetworkingObjects::NetObjID& objectID) {
    class DeleteBookmarkWorldUndoAction : public WorldUndoAction {
        public:
            DeleteBookmarkWorldUndoAction(std::unique_ptr<BookmarkCompleteUndoData> initBookmarkData, uint32_t newPos, WorldUndoManager::UndoObjectID initParentUndoID, WorldUndoManager::UndoObjectID initUndoID):
                bookmarkData(std::move(initBookmarkData)),
                pos(newPos),
                parentUndoID(initParentUndoID),
                undoID(initUndoID)
            {}
            std::string get_name() const override {
                return "Delete Bookmark Item";
            }
            bool undo(WorldUndoManager& undoMan) override {
                std::optional<NetworkingObjects::NetObjID> toInsertParentID = undoMan.get_netid_from_undoid(parentUndoID);
                if(!toInsertParentID.has_value())
                    return false;
                std::optional<NetworkingObjects::NetObjID> toInsertID = undoMan.get_netid_from_undoid(undoID);
                if(toInsertID.has_value())
                    return false;

                auto& bookmarkList = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(toInsertParentID.value())->get_folder_list();
                auto insertedIt = bookmarkList->emplace_direct(bookmarkList, bookmarkList->at(pos), undoMan.world, *bookmarkData);
                undoMan.register_new_netid_to_existing_undoid(undoID, insertedIt->obj.get_net_id());
                bookmarkData = nullptr;
                return true;
            }
            bool redo(WorldUndoManager& undoMan) override {
                std::optional<NetworkingObjects::NetObjID> toEraseParentID = undoMan.get_netid_from_undoid(parentUndoID);
                if(!toEraseParentID.has_value())
                    return false;
                std::optional<NetworkingObjects::NetObjID> toEraseID = undoMan.get_netid_from_undoid(undoID);
                if(!toEraseID.has_value())
                    return false;

                auto& bookmarkList = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(toEraseParentID.value())->get_folder_list();
                auto it = bookmarkList->get(toEraseID.value());
                bookmarkData = std::make_unique<BookmarkCompleteUndoData>(it->obj->get_complete_undo_data(undoMan));
                bookmarkList->erase(bookmarkList, it);
                return true;
            }
            void scale_up(const WorldScalar& scaleAmount) override {
                if(bookmarkData)
                    bookmarkData->scale_up(scaleAmount);
            }
            ~DeleteBookmarkWorldUndoAction() {}

            std::unique_ptr<BookmarkCompleteUndoData> bookmarkData;
            uint32_t pos;
            WorldUndoManager::UndoObjectID parentUndoID;
            WorldUndoManager::UndoObjectID undoID;
    };

    auto& folderList = world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(parentID)->get_folder_list();
    auto it = folderList->get(objectID);
    world.undo.push(std::make_unique<DeleteBookmarkWorldUndoAction>(std::make_unique<BookmarkCompleteUndoData>(it->obj->get_complete_undo_data(world.undo)), it->pos, world.undo.get_undoid_from_netid(parentID), world.undo.get_undoid_from_netid(objectID)));
    folderList->erase(folderList, it);
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

void BookmarkManager::editing_bookmark_check() {
    class EditBookmarkWorldUndoAction : public WorldUndoAction {
        public:
            EditBookmarkWorldUndoAction(const std::string& initName, WorldUndoManager::UndoObjectID initUndoID):
                nameData(initName),
                undoID(initUndoID)
            {}
            std::string get_name() const override {
                return "Edit Bookmark";
            }
            bool undo(WorldUndoManager& undoMan) override {
                return undo_redo(undoMan);
            }
            bool redo(WorldUndoManager& undoMan) override {
                return undo_redo(undoMan);
            }
            bool undo_redo(WorldUndoManager& undoMan) {
                std::optional<NetworkingObjects::NetObjID> toModifyID = undoMan.get_netid_from_undoid(undoID);
                if(!toModifyID.has_value())
                    return false;
                auto objPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(toModifyID.value());
                std::string newNameData = objPtr->get_name();
                objPtr->set_name(undoMan.world.delayedUpdateObjectManager, nameData);
                nameData = newNameData;
                return true;
            }
            ~EditBookmarkWorldUndoAction() {}

            std::string nameData;
            WorldUndoManager::UndoObjectID undoID;
    };

    if(oldSelection.size() == 1 && editingBookmarkName.has_value()) {
        const NetworkingObjects::NetObjID& oldNetObjID = *oldSelection.begin();
        auto tempPtr = world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(oldNetObjID);
        if(tempPtr) {
            const std::string& currentNameData = tempPtr->get_name();
            if(currentNameData != editingBookmarkName.value())
                world.undo.push(std::make_unique<EditBookmarkWorldUndoAction>(editingBookmarkName.value(), world.undo.get_undoid_from_netid(oldNetObjID)));
        }
    }
    editingBookmarkName = std::nullopt;
    if(selectionData.objsSelected.size() == 1) {
        auto tempPtr = world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(*selectionData.objsSelected.begin());
        if(tempPtr)
            editingBookmarkName = tempPtr->get_name();
    }
    oldSelection = selectionData.objsSelected;
}
