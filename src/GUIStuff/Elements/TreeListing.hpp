#pragma once
#include "Element.hpp"
#include <Helpers/NetworkingObjects/NetObjID.hpp>
#include <set>

namespace GUIStuff {
typedef std::vector<size_t> TreeListingObjIndexList;
}

bool operator<(const GUIStuff::TreeListingObjIndexList& a, const GUIStuff::TreeListingObjIndexList& b);

namespace GUIStuff {

class GUIManager;

class TreeListing : public Element {
    public:
        static constexpr float ENTRY_HEIGHT = 25.0f;
        static constexpr float ICON_SIZE = 25.0f;

        struct ObjInfo {
            TreeListingObjIndexList objIndex;
            bool isDirectory = false;
            bool isOpen = false;
        };

        struct DirectoryInfo {
            size_t dirSize;
            bool isOpen;
        };

        struct Data {
            std::set<TreeListingObjIndexList>* selectedIndices = nullptr;
            std::function<std::optional<DirectoryInfo>(const TreeListingObjIndexList& objIndex)> dirInfo;
            std::function<void(const TreeListingObjIndexList& objIndex, bool isOpen)> setDirectoryOpen;
            std::function<void(const TreeListingObjIndexList& objIndex)> drawNonDirectoryObjIconGUI;
            std::function<void(const TreeListingObjIndexList& objIndex)> drawObjGUI;
            std::function<void(const TreeListingObjIndexList& objIndex)> onDoubleClick;
            std::function<void()> onSelectChange;
            std::function<void(const std::vector<TreeListingObjIndexList>& objIndices, const TreeListingObjIndexList& newObjIndex)> moveObj;
        };

        TreeListing(GUIManager& gui);
        void layout(const Clay_ElementId& id, const Data& newDisplayData);
    private:
        void recursive_visible_flattened_obj_list(std::vector<ObjInfo>& objs, const TreeListingObjIndexList& objIndex);

        std::vector<ObjInfo> flattenedIndexList;
        Data d;
};

}
