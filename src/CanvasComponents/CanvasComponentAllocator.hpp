#pragma once
#include <memory>
#include "CanvasComponentType.hpp"
#include <Helpers/NetworkingObjects/NetObjTemporaryPtr.hpp>
#include <Helpers/NetworkingObjects/DelayUpdateSerializedClassManager.hpp>
#include <Helpers/VersionNumber.hpp>

class World;
class CanvasComponent;

class CanvasComponentAllocator {
    public:
        CanvasComponentAllocator();
        CanvasComponentAllocator(CanvasComponentType typeToAllocate);
        std::unique_ptr<CanvasComponent> comp;
        static void register_class(World& world);
        void save_file(cereal::PortableBinaryOutputArchive& a) const;
        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version);
};
