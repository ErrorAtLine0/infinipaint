#include "CanvasComponentAllocator.hpp"
#include "CanvasComponentContainer.hpp"
#include "../DrawingProgram/DrawingProgram.hpp"

CanvasComponentAllocator::CanvasComponentAllocator() {}

CanvasComponentAllocator::CanvasComponentAllocator(CanvasComponent::CompType typeToAllocate) {
    comp = std::unique_ptr<CanvasComponent>(CanvasComponent::allocate_comp(typeToAllocate));
}

void CanvasComponentAllocator::register_class(DrawingProgram& drawP, NetworkingObjects::DelayUpdateSerializedClassManager& delayUpdateMan, NetworkingObjects::NetObjManager& objMan) {
    delayUpdateMan.register_class<CanvasComponentAllocator>(objMan, NetworkingObjects::DelayUpdateSerializedClassManager::CustomConstructors<CanvasComponentAllocator>{
        .writeConstructor = [](const CanvasComponentAllocator& o, cereal::PortableBinaryOutputArchive& a) {
            a(o.comp->get_type());
            a(*o.comp);
        },
        .readConstructor = [](CanvasComponentAllocator& o, cereal::PortableBinaryOutputArchive& a) {
            CanvasComponent::CompType typeToAllocate;
            a(typeToAllocate);
            o.comp = std::unique_ptr<CanvasComponent>(CanvasComponent::allocate_comp(typeToAllocate));
            a(*o.comp);
        },
        .writeUpdate = [](const CanvasComponentAllocator& o, cereal::PortableBinaryOutputArchive& a) {
            a(*o.comp);
        },
        .readUpdate = [](CanvasComponentAllocator& o, cereal::PortableBinaryOutputArchive& a) {
            a(*o.comp);
        },
        .allocateCopy = [](const CanvasComponentAllocator& o) {
            auto theCopy = std::make_shared<CanvasComponentAllocator>();
            theCopy->comp = std::unique_ptr<CanvasComponent>(CanvasComponent::allocate_comp(o.comp->get_type()));
            // Maybe do an assignment here
            return theCopy;
        },
        .assignmentFunc = [](CanvasComponentAllocator& o, const CanvasComponentAllocator& o2) {
            // Assignment still needs to be implemented
        },
        .postUpdateFunc = [&drawP](CanvasComponentAllocator& o) {
            o.comp->compContainer->commit_update(drawP);
        }
    });
}
