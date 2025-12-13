#pragma once
#include <memory>
#include "CanvasComponent.hpp"
#include <Helpers/NetworkingObjects/NetObjTemporaryPtr.hpp>

class CanvasComponentAllocator {
    public:
        CanvasComponentAllocator();
        CanvasComponentAllocator(CanvasComponent::CompType typeToAllocate);
        std::unique_ptr<CanvasComponent> comp;
        static void register_class(NetworkingObjects::NetObjManager& objMan);
    private:
        static void write_constructor_func(const NetworkingObjects::NetObjTemporaryPtr<CanvasComponentAllocator>& o, cereal::PortableBinaryOutputArchive& a);
        static void read_constructor_func(const NetworkingObjects::NetObjTemporaryPtr<CanvasComponentAllocator>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c);
};
