#include "BookmarkListItem.hpp"
#include <cereal/types/string.hpp>
#include "../World.hpp"
#include "../ScaleUpCanvas.hpp"
#include "BookmarkManager.hpp"
#include "../World.hpp"

using namespace NetworkingObjects;

void BookmarkData::jump_to(World& world) const {
    world.drawData.cam.smooth_move_to(world, coords, windowSize.cast<float>());
}

BookmarkListItem::BookmarkListItem() {}

BookmarkListItem::BookmarkListItem(NetworkingObjects::NetObjManager& netObjMan, const std::string& initName, bool isFolder, const BookmarkData& initBookmarkData) {
    if(isFolder) {
        folderData = std::make_unique<BookmarkFolderData>();
        folderData->folderList = netObjMan.make_obj<NetworkingObjects::NetObjOrderedList<BookmarkListItem>>();
    }
    else
        bookmarkData = std::make_unique<BookmarkData>(initBookmarkData);
    nameData = netObjMan.make_obj<NameData>();
    nameData->name = initName;
}

void BookmarkListItem::reassign_netobj_ids_call() {
    if(folderData)
        folderData->folderList.reassign_ids();
    nameData.reassign_ids();
}

void BookmarkListItem::scale_up(const WorldScalar& scaleUpAmount) {
    if(folderData) {
        for(auto& i : *folderData->folderList)
            i.obj->scale_up(scaleUpAmount);
    }
    else
        bookmarkData->coords.scale_about(WorldVec{0, 0}, scaleUpAmount, true);
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

void BookmarkListItem::set_name(NetworkingObjects::DelayUpdateSerializedClassManager& delayedNetObjMan, const std::string& newName) {
    if(nameData && nameData->name != newName) {
        nameData->name = newName;
        delayedNetObjMan.send_update_to_all<NameData>(nameData, false);
    }
}

const std::string& BookmarkListItem::get_name() const {
    return nameData->name;
}

bool BookmarkListItem::is_folder_open() const {
    return folderData->isFolderOpen;
}

void BookmarkListItem::set_folder_open(bool newIsFolderOpen) {
    folderData->isFolderOpen = newIsFolderOpen;
}

void BookmarkListItem::register_class(World& w) {
    auto readConstructorData = [&w](const NetworkingObjects::NetObjTemporaryPtr<BookmarkListItem>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) {
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
        o->nameData = o.get_obj_man()->read_create_message<NameData>(a, c);

        canvas_scale_up_check(*o, w, c);
    };
    w.netObjMan.register_class<BookmarkListItem, BookmarkListItem, BookmarkListItem, BookmarkListItem>({
        .writeConstructorFuncClient = write_constructor_data,
        .readConstructorFuncClient = readConstructorData,
        .readUpdateFuncClient = nullptr,
        .writeConstructorFuncServer = write_constructor_data,
        .readConstructorFuncServer = readConstructorData,
        .readUpdateFuncServer = nullptr
    });
    NetworkingObjects::register_ordered_list_class<BookmarkListItem>(w.netObjMan);
    w.delayedUpdateObjectManager.register_class<NameData>(w.netObjMan);
}

void BookmarkListItem::write_constructor_data(const NetObjTemporaryPtr<BookmarkListItem>& o, cereal::PortableBinaryOutputArchive& a) {
    a(o->is_folder());
    if(o->is_folder())
        o->folderData->folderList.write_create_message(a);
    else
        a(*o->bookmarkData);
    o->nameData.write_create_message(a);
}

void BookmarkListItem::save_file(cereal::PortableBinaryOutputArchive& a) const {
    a(*nameData);
    a(static_cast<bool>(folderData));
    if(folderData) {
        a(folderData->folderList->size());
        for(auto& item : *folderData->folderList)
            item.obj->save_file(a);
    }
    else
        a(*bookmarkData);
}

void BookmarkListItem::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version, BookmarkManager& bMan) {
    nameData = bMan.world.netObjMan.make_obj<NameData>();
    a(*nameData);

    bool isFolder;
    a(isFolder);
    if(isFolder) {
        folderData = std::make_unique<BookmarkFolderData>();
        folderData->folderList = bMan.world.netObjMan.make_obj<NetworkingObjects::NetObjOrderedList<BookmarkListItem>>();
        uint32_t folderSize;
        a(folderSize);
        for(uint32_t i = 0; i < folderSize; i++) {
            BookmarkListItem* item = new BookmarkListItem;
            item->load_file(a, version, bMan);
            folderData->folderList->push_back_and_send_create(folderData->folderList, item);
        }
    }
    else {
        bookmarkData = std::make_unique<BookmarkData>();
        a(*bookmarkData);
    }
}
