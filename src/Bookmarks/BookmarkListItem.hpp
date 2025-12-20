#pragma once
#include <Helpers/NetworkingObjects/NetObjOwnerPtr.hpp>
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include <string>
#include "../CoordSpaceHelper.hpp"

struct BookmarkData {
    CoordSpaceHelper coords;
    Vector<int32_t, 2> windowSize;
    template <class Archive> void serialize(Archive& a) {
        a(coords, windowSize);
    }
    void jump_to(World& world);
};

class BookmarkListItem {
    public:
        BookmarkListItem();
        BookmarkListItem(NetworkingObjects::NetObjManager& netObjMan, const std::string& initName, bool isFolder, const BookmarkData& initBookmarkData);
        bool is_folder() const;
        const BookmarkData& get_bookmark_data() const;
        const NetworkingObjects::NetObjOwnerPtr<NetworkingObjects::NetObjOrderedList<BookmarkListItem>>& get_folder_list() const;
        bool is_folder_open() const;
        void set_folder_open(bool newIsFolderOpen);
        const std::string& get_name() const;
        static void register_class(NetworkingObjects::NetObjManager& netObjMan);
        static void set_name(const NetworkingObjects::NetObjTemporaryPtr<BookmarkListItem>& o, const std::string& newName); // Set once after editing is finished
    private:
        struct BookmarkFolderData {
            NetworkingObjects::NetObjOwnerPtr<NetworkingObjects::NetObjOrderedList<BookmarkListItem>> folderList;
            bool isFolderOpen = false;
        };
        static void write_constructor_data(const NetworkingObjects::NetObjTemporaryPtr<BookmarkListItem>& o, cereal::PortableBinaryOutputArchive& a);
        static void read_constructor_data(const NetworkingObjects::NetObjTemporaryPtr<BookmarkListItem>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c);
        std::unique_ptr<BookmarkFolderData> folderData;
        std::unique_ptr<BookmarkData> bookmarkData;
        std::string name;
};
