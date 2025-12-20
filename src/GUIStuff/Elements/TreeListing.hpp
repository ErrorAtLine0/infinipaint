#pragma once
#include "Element.hpp"
#include <Helpers/NetworkingObjects/NetObjID.hpp>

namespace GUIStuff {

class GUIManager;

class TreeListing : public Element {
    public:
        static constexpr float ENTRY_HEIGHT = 25.0f;
        struct DisplayData {
            struct ObjInList {
                NetworkingObjects::NetObjID id;
                bool isDirectory = false;
                bool isDirectoryOpen = false;
            };
            std::function<std::optional<ObjInList>(NetworkingObjects::NetObjID, size_t)> getObjInListAtIndex;
            std::function<void(NetworkingObjects::NetObjID, bool)> setDirectoryOpen;
            std::function<bool(NetworkingObjects::NetObjID)> drawNonDirectoryObjIconGUI;
            std::function<bool(NetworkingObjects::NetObjID, NetworkingObjects::NetObjID)> drawObjGUI;
        };
        void update(UpdateInputData& io, GUIManager& gui, NetworkingObjects::NetObjID rootObjID, const DisplayData& displayData);
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) override;
    private:
        void recursive_gui(UpdateInputData& io, GUIManager& gui, const DisplayData& displayData, NetworkingObjects::NetObjID objID, size_t& itemCount, size_t itemCountToStartAt, size_t itemCountToEndAt, size_t listDepth);
        std::optional<NetworkingObjects::NetObjID> objSelected;
};

}
