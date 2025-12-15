#include "CanvasComponentAllocator.hpp"
#include "CanvasComponentContainer.hpp"
#include "../DrawingProgram/DrawingProgram.hpp"
#include "../World.hpp"
#include <Helpers/NetworkingObjects/DelayUpdateSerializedClassManager.hpp>
#include "CanvasComponent.hpp"

CanvasComponentAllocator::CanvasComponentAllocator() {}

CanvasComponentAllocator::CanvasComponentAllocator(CanvasComponentType typeToAllocate) {
    comp = std::unique_ptr<CanvasComponent>(CanvasComponent::allocate_comp(typeToAllocate));
}

void CanvasComponentAllocator::register_class(World& world, NetworkingObjects::NetObjManager& objMan) {
    world.delayedUpdateObjectManager.register_class<CanvasComponentAllocator>(objMan, NetworkingObjects::DelayUpdateSerializedClassManager::CustomConstructors<CanvasComponentAllocator>{
        .writeConstructor = [](const CanvasComponentAllocator& o, cereal::PortableBinaryOutputArchive& a) {
            a(o.comp->get_type());
            a(*o.comp);
        },
        .readConstructor = [](CanvasComponentAllocator& o, cereal::PortableBinaryInputArchive& a) {
            CanvasComponentType typeToAllocate;
            a(typeToAllocate);
            o.comp = std::unique_ptr<CanvasComponent>(CanvasComponent::allocate_comp(typeToAllocate));
            a(*o.comp);
        },
        .writeUpdate = [](const CanvasComponentAllocator& o, cereal::PortableBinaryOutputArchive& a) {
            a(*o.comp);
        },
        .readUpdate = [](CanvasComponentAllocator& o, cereal::PortableBinaryInputArchive& a) {
            a(*o.comp);
        },
        .allocateCopy = [](const CanvasComponentAllocator& o) {
            auto theCopy = std::make_shared<CanvasComponentAllocator>();
            theCopy->comp = std::unique_ptr<CanvasComponent>(CanvasComponent::allocate_comp(o.comp->get_type()));
            // Maybe do an assignment here
            return theCopy;
        },
        .assignmentFunc = [](CanvasComponentAllocator& o, const CanvasComponentAllocator& o2) {
            o.comp->set_data_from(*o2.comp);
        },
        .postUpdateFunc = [&drawP = world.drawProg](CanvasComponentAllocator& o) {
            o.comp->compContainer->commit_update(drawP);
        }
    });
}
