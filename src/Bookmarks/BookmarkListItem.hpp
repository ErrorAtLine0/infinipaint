#pragma once
#include <Helpers/NetworkingObjects/NetObjOwnerPtr.hpp>
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include <Helpers/NetworkingObjects/DelayUpdateSerializedClassManager.hpp>
#include <string>
#include "../WorldUndoManager.hpp"
#include "../CoordSpaceHelper.hpp"
#include <Helpers/VersionNumber.hpp>

class BookmarkManager;

struct BookmarkData {
    CoordSpaceHelper coords;
    Vector<int32_t, 2> windowSize;
    template <class Archive> void serialize(Archive& a) {
        a(coords, windowSize);
    }
    void scale_up(const WorldScalar& scaleUpAmount);
    void jump_to(World& world) const;
};

struct BookmarkCompleteUndoData {
    WorldUndoManager::UndoObjectID undoID;
    std::string name;
    std::optional<std::vector<BookmarkCompleteUndoData>> folderList;
    std::optional<BookmarkData> bookmarkData;
    void scale_up(const WorldScalar& scaleUpAmount);
};

class BookmarkListItem {
    public:
        BookmarkListItem();
        BookmarkListItem(World& w, const BookmarkCompleteUndoData& undoData);
        BookmarkListItem(NetworkingObjects::NetObjManager& netObjMan, const std::string& initName, bool isFolder, const BookmarkData& initBookmarkData);
        bool is_folder() const;
        const BookmarkData& get_bookmark_data() const;
        const NetworkingObjects::NetObjOwnerPtr<NetworkingObjects::NetObjOrderedList<BookmarkListItem>>& get_folder_list() const;
        bool is_folder_open() const;
        void set_folder_open(bool newIsFolderOpen);
        const std::string& get_name() const;
        static void register_class(World& w);
        void reassign_netobj_ids_call();
        void scale_up(const WorldScalar& scaleUpAmount);
        void save_file(cereal::PortableBinaryOutputArchive& a) const;
        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version, BookmarkManager& bMan);

        BookmarkCompleteUndoData get_complete_undo_data(WorldUndoManager& u);
        
        void set_name(NetworkingObjects::DelayUpdateSerializedClassManager& delayedNetObjMan, const std::string& newName);
    private:
        struct BookmarkFolderData {
            NetworkingObjects::NetObjOwnerPtr<NetworkingObjects::NetObjOrderedList<BookmarkListItem>> folderList;
            bool isFolderOpen = false;
        };
        struct NameData {
            std::string name;
            template <typename Archive> void serialize(Archive& a) {
                a(name);
            }
        };
        static void write_constructor_data(const NetworkingObjects::NetObjTemporaryPtr<BookmarkListItem>& o, cereal::PortableBinaryOutputArchive& a);
        std::unique_ptr<BookmarkFolderData> folderData;
        std::unique_ptr<BookmarkData> bookmarkData;
        NetworkingObjects::NetObjOwnerPtr<NameData> nameData;
};
