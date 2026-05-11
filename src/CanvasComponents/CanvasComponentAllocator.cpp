/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

void CanvasComponentAllocator::register_class(World& world) {
    world.delayedUpdateObjectManager.register_class<CanvasComponentAllocator>(world.netObjMan, NetworkingObjects::DelayUpdateSerializedClassManager::CustomConstructors<CanvasComponentAllocator>{
        .writeConstructor = [](const CanvasComponentAllocator& o, cereal::PortableBinaryOutputArchive& a) {
            a(o.comp->get_type());
            a(*o.comp);
        },
        .readConstructor = [](CanvasComponentAllocator& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) {
            CanvasComponentType typeToAllocate;
            a(typeToAllocate);
            o.comp = std::unique_ptr<CanvasComponent>(CanvasComponent::allocate_comp(typeToAllocate));
            a(*o.comp);
        },
        .writeUpdate = [](const CanvasComponentAllocator& o, cereal::PortableBinaryOutputArchive& a) {
            a(*o.comp);
        },
        .readUpdate = [](CanvasComponentAllocator& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) {
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

void CanvasComponentAllocator::save_file(cereal::PortableBinaryOutputArchive& a) const {
    a(comp->get_type());
    comp->save_file(a);
}

void CanvasComponentAllocator::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    CanvasComponentType t;
    a(t);
    comp = std::unique_ptr<CanvasComponent>(CanvasComponent::allocate_comp(t));
    comp->load_file(a, version);
}
