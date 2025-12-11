#pragma once
#include "../CoordSpaceHelper.hpp"
#include "CanvasComponent.hpp"
#include <Helpers/NetworkingObjects/NetObjManagerTypeList.hpp>
#include "../VersionConstants.hpp"

class CanvasComponentContainer {
    public:
        CanvasComponentContainer();
        CanvasComponentContainer(CanvasComponent::CompType type);
        static void register_class(NetworkingObjects::NetObjManagerTypeList& t);
        void save_file(cereal::PortableBinaryOutputArchive& a) const;
        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version);
        const std::unique_ptr<CanvasComponent>& get_comp();
        const CoordSpaceHelper& get_coords();
    private:
        std::unique_ptr<CanvasComponent> allocate_comp(CanvasComponent::CompType type);
        std::unique_ptr<CanvasComponent> comp;
        CoordSpaceHelper coords;
};
