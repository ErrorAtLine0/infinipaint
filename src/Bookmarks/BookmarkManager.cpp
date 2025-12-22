#include "BookmarkManager.hpp"
#include "../World.hpp"
#include "../MainProgram.hpp"
#include "Helpers/NetworkingObjects/NetObjOrderedList.hpp"
#include "Helpers/NetworkingObjects/NetObjTemporaryPtr.decl.hpp"

BookmarkManager::BookmarkManager(World& w):
    world(w)
{}

void BookmarkManager::init() {
    // NOTE: Should find a way for bookmarks added from the network to take into account the client's canvasScale
    // Have the canvasScale variable come with the ClientData object, and the grid scale commands can be part of 
    // the client data class
    if(world.netObjMan.is_server()) {
        bookmarkListRoot = world.netObjMan.make_obj_from_ptr<BookmarkListItem>(new BookmarkListItem(world.netObjMan, "ROOT", true, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 1", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 2", false, {}));
        auto folder1Ptr = bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Folder 1", true, {}));
            folder1Ptr->obj->get_folder_list()->push_back_and_send_create(folder1Ptr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark A", false, {}));
            folder1Ptr->obj->get_folder_list()->push_back_and_send_create(folder1Ptr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark B", false, {}));
            folder1Ptr->obj->get_folder_list()->push_back_and_send_create(folder1Ptr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark C", false, {}));
            folder1Ptr->obj->get_folder_list()->push_back_and_send_create(folder1Ptr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark D", false, {}));
            folder1Ptr->obj->get_folder_list()->push_back_and_send_create(folder1Ptr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark E", false, {}));
            auto folderAPtr = folder1Ptr->obj->get_folder_list()->push_back_and_send_create(folder1Ptr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Folder A", true, {}));
                folderAPtr->obj->get_folder_list()->push_back_and_send_create(folderAPtr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 1", false, {}));
                folderAPtr->obj->get_folder_list()->push_back_and_send_create(folderAPtr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 2", false, {}));
                folderAPtr->obj->get_folder_list()->push_back_and_send_create(folderAPtr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 3", false, {}));
                folderAPtr->obj->get_folder_list()->push_back_and_send_create(folderAPtr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 4", false, {}));
                folderAPtr->obj->get_folder_list()->push_back_and_send_create(folderAPtr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 5", false, {}));
                folderAPtr->obj->get_folder_list()->push_back_and_send_create(folderAPtr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 6", false, {}));
                folderAPtr->obj->get_folder_list()->push_back_and_send_create(folderAPtr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 7", false, {}));
            folder1Ptr->obj->get_folder_list()->push_back_and_send_create(folder1Ptr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark F", false, {}));
            folder1Ptr->obj->get_folder_list()->push_back_and_send_create(folder1Ptr->obj->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark G", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 3", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 4", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 5", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 6", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 7", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 8", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 9", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 10", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 11", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 12", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 13", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 14", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 15", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 16", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 17", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 18", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 19", false, {}));
        bookmarkListRoot->get_folder_list()->push_back_and_send_create(bookmarkListRoot->get_folder_list(), new BookmarkListItem(world.netObjMan, "Bookmark 20", false, {}));
    }
}

void BookmarkManager::setup_list_gui(const std::string& id) {
    using namespace NetworkingObjects;
    if(bookmarkListRoot) {
        std::optional<NetObjID> toDeleteParent;
        std::optional<NetObjID> toDeleteObject;
        auto& gui = world.main.toolbar.gui;
        gui.tree_listing(id, bookmarkListRoot.get_net_id(), GUIStuff::TreeListing::DisplayData{
            .getObjInListAtIndex = [&](NetObjID parentId, size_t index) -> std::optional<GUIStuff::TreeListing::DisplayData::ObjInList> {
                NetObjOrderedList<BookmarkListItem>& bookmarkParentFolder = *world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(parentId)->get_folder_list();
                if(bookmarkParentFolder.size() <= index)
                    return std::nullopt;
                return GUIStuff::TreeListing::DisplayData::ObjInList{
                    .id = bookmarkParentFolder.at(index)->obj.get_net_id(),
                    .isDirectory = bookmarkParentFolder.at(index)->obj->is_folder(),
                    .isDirectoryOpen = bookmarkParentFolder.at(index)->obj->is_folder() ? bookmarkParentFolder.at(index)->obj->is_folder_open() : false
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
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                        .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                    },
                }) {
                    gui.text_label(world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(idPair.object)->get_name());
                }
                if(gui.svg_icon_button_transparent("delete button", "data/icons/trash.svg", false, GUIStuff::TreeListing::ENTRY_HEIGHT)) {
                    toDeleteParent = idPair.parent;
                    toDeleteObject = idPair.object;
                }
                return false;
            },
            .moveObjectsToListAtIndex = [&](NetObjID listObj, size_t index, const std::vector<GUIStuff::TreeListing::ParentObjectIDPair>& objsToInsert) {
                std::unordered_map<NetObjID, std::unordered_set<NetObjID>> toEraseMap;
                uint32_t newIndex = index;
                std::vector<NetObjOwnerPtr<BookmarkListItem>> toInsertObjPtrs;
                auto& listPtr = world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(listObj)->get_folder_list();

                for(size_t i = 0; i < objsToInsert.size(); i++) {
                    toEraseMap[objsToInsert[i].parent].emplace(objsToInsert[i].object);
                    if(newIndex != 0 && objsToInsert[i].parent == listObj && listPtr->get(objsToInsert[i].object)->pos <= index)
                        newIndex--;
                }

                for(auto& [listToEraseFrom, setToErase] : toEraseMap) {
                    auto& listToEraseFromPtr = world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(listToEraseFrom)->get_folder_list();
                    listToEraseFromPtr->erase_unordered_set(listToEraseFromPtr, setToErase, &toInsertObjPtrs);
                }

                std::vector<std::pair<uint32_t, NetObjOwnerPtr<BookmarkListItem>>> toInsert;
                for(uint32_t i = 0; i < objsToInsert.size(); i++) {
                    auto it = std::find_if(toInsertObjPtrs.begin(), toInsertObjPtrs.end(), [id = objsToInsert[i].object](NetObjOwnerPtr<BookmarkListItem>& objPtr) {
                        return objPtr.get_net_id() == id;
                    });
                    toInsert.emplace_back(newIndex + i, std::move(*it));
                    toInsert.back().second.reassign_ids();
                }
                listPtr->insert_ordered_list_and_send_create(listPtr, toInsert);
            }
        });
        if(toDeleteObject.has_value()) {
            auto& folderList = world.netObjMan.get_obj_temporary_ref_from_id<BookmarkListItem>(toDeleteParent.value())->get_folder_list();
            folderList->erase(folderList, toDeleteObject.value());
        }
    }
}

void BookmarkManager::scale_up(const WorldScalar& scaleUpAmount) {
}
