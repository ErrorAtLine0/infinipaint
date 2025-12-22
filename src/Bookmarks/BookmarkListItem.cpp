#include "BookmarkListItem.hpp"
#include <cereal/types/string.hpp>
#include "../World.hpp"

using namespace NetworkingObjects;

void BookmarkData::jump_to(World& world) {
    world.drawData.cam.smooth_move_to(world, coords, windowSize.cast<float>());
}

BookmarkListItem::BookmarkListItem() {}

BookmarkListItem::BookmarkListItem(NetworkingObjects::NetObjManager& netObjMan, const std::string& initName, bool isFolder, const BookmarkData& initBookmarkData) {
    name = initName;
    if(isFolder) {
        folderData = std::make_unique<BookmarkFolderData>();
        folderData->folderList = netObjMan.make_obj<NetworkingObjects::NetObjOrderedList<BookmarkListItem>>();
    }
    else
        bookmarkData = std::make_unique<BookmarkData>(initBookmarkData);
}

void BookmarkListItem::reassign_netobj_ids_call() {
    if(folderData)
        folderData->folderList.reassign_ids();
}

bool BookmarkListItem::is_folder() const {
    return folderData != nullptr;
}

const BookmarkData& BookmarkListItem::get_bookmark_data() const {
    return *bookmarkData;
}

const NetObjOwnerPtr<NetObjOrderedList<BookmarkListItem>>& BookmarkListItem::get_folder_list() const {
    return folderData->folderList;
}

const std::string& BookmarkListItem::get_name() const {
    return name;
}

bool BookmarkListItem::is_folder_open() const {
    return folderData->isFolderOpen;
}

void BookmarkListItem::set_folder_open(bool newIsFolderOpen) {
    folderData->isFolderOpen = newIsFolderOpen;
}

void BookmarkListItem::set_name(const NetworkingObjects::NetObjTemporaryPtr<BookmarkListItem>& o, const std::string& newName) {
    o->name = newName;
    o.send_update_to_all(RELIABLE_COMMAND_CHANNEL, [](const NetObjTemporaryPtr<BookmarkListItem>& o, cereal::PortableBinaryOutputArchive& a) {
        a(o->name);
    });
}

void BookmarkListItem::register_class(NetObjManager& netObjMan) {
    netObjMan.register_class<BookmarkListItem, BookmarkListItem, BookmarkListItem, BookmarkListItem>({
        .writeConstructorFuncClient = write_constructor_data,
        .readConstructorFuncClient = read_constructor_data,
        .readUpdateFuncClient = [](const NetObjTemporaryPtr<BookmarkListItem>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) {
            a(o->name);
        },
        .writeConstructorFuncServer = write_constructor_data,
        .readConstructorFuncServer = read_constructor_data,
        .readUpdateFuncServer = [](const NetObjTemporaryPtr<BookmarkListItem>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) {
            a(o->name);
            o.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [](const NetObjTemporaryPtr<BookmarkListItem>& o, cereal::PortableBinaryOutputArchive& a) {
                a(o->name);
            });
        },
    });
}

void BookmarkListItem::write_constructor_data(const NetObjTemporaryPtr<BookmarkListItem>& o, cereal::PortableBinaryOutputArchive& a) {
    a(o->name, o->is_folder());
    if(o->is_folder())
        o->folderData->folderList.write_create_message(a);
    else
        a(*o->bookmarkData);
}

void BookmarkListItem::read_constructor_data(const NetworkingObjects::NetObjTemporaryPtr<BookmarkListItem>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) {
    a(o->name);
    bool isFolder;
    a(isFolder);
    if(isFolder) {
        o->folderData = std::make_unique<BookmarkFolderData>();
        o->folderData->folderList = o.get_obj_man()->read_create_message<NetworkingObjects::NetObjOrderedList<BookmarkListItem>>(a, c);
    }
    else {
        o->bookmarkData = std::make_unique<BookmarkData>();
        a(*o->bookmarkData);
    }
}
