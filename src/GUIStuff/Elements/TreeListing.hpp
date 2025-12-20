#pragma once
#include "Element.hpp"
#include "../../Bookmarks/BookmarkListItem.hpp"
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>

namespace GUIStuff {

class GUIManager;

class TreeListing : public Element {
    public:
        void update(UpdateInputData& io, GUIManager& gui, const NetworkingObjects::NetObjTemporaryPtr<NetworkingObjects::NetObjOrderedList<BookmarkListItem>>& bookmarkList);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) override;
    private:
        void recursive_gui(UpdateInputData& io, GUIManager& gui, const NetworkingObjects::NetObjTemporaryPtr<NetworkingObjects::NetObjOrderedList<BookmarkListItem>>& bookmarkList, size_t& itemCount, size_t itemCountToStartAt, size_t itemCountToEndAt, size_t listDepth);
        std::optional<NetworkingObjects::NetObjID> objSelected;
};

}
