#pragma once
#include <memory>
#include "CanvasComponentType.hpp"
#include <Helpers/NetworkingObjects/NetObjTemporaryPtr.hpp>
#include <Helpers/NetworkingObjects/DelayUpdateSerializedClassManager.hpp>

class World;
class CanvasComponent;

class CanvasComponentAllocator {
    public:
        CanvasComponentAllocator();
        CanvasComponentAllocator(CanvasComponentType typeToAllocate);
        std::unique_ptr<CanvasComponent> comp;
        static void register_class(World& world);
};
