#pragma once
#include <memory>
#include "CanvasComponent.hpp"
#include <Helpers/NetworkingObjects/NetObjTemporaryPtr.hpp>
#include <Helpers/NetworkingObjects/DelayUpdateSerializedClassManager.hpp>

class DrawingProgram;

class CanvasComponentAllocator {
    public:
        CanvasComponentAllocator();
        CanvasComponentAllocator(CanvasComponent::CompType typeToAllocate);
        std::unique_ptr<CanvasComponent> comp;
        static void register_class(DrawingProgram& drawP, NetworkingObjects::DelayUpdateSerializedClassManager& delayUpdateMan, NetworkingObjects::NetObjManager& objMan);
};
