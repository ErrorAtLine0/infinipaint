#include "CanvasComponentAllocator.hpp"

CanvasComponentAllocator::CanvasComponentAllocator() {}

CanvasComponentAllocator::CanvasComponentAllocator(CanvasComponent::CompType typeToAllocate) {
    comp = std::unique_ptr<CanvasComponent>(CanvasComponent::allocate_comp(typeToAllocate));
}

void CanvasComponentAllocator::register_class(NetworkingObjects::NetObjManager& objMan) {
    objMan.register_class<CanvasComponentAllocator, CanvasComponentAllocator, CanvasComponentAllocator, CanvasComponentAllocator>({
        .writeConstructorFuncClient = write_constructor_func,
        .readConstructorFuncClient = read_constructor_func,
        .readUpdateFuncClient = nullptr,
        .writeConstructorFuncServer = write_constructor_func,
        .readConstructorFuncServer = read_constructor_func,
        .readUpdateFuncServer = nullptr,
    });
}

void CanvasComponentAllocator::write_constructor_func(const NetworkingObjects::NetObjTemporaryPtr<CanvasComponentAllocator>& o, cereal::PortableBinaryOutputArchive& a) {
    a(o->comp->get_type());
    a(*o->comp);
}

void CanvasComponentAllocator::read_constructor_func(const NetworkingObjects::NetObjTemporaryPtr<CanvasComponentAllocator>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) {
    CanvasComponent::CompType typeToAllocate;
    a(typeToAllocate);
    o->comp = std::unique_ptr<CanvasComponent>(CanvasComponent::allocate_comp(typeToAllocate));
    a(*o->comp);
}
