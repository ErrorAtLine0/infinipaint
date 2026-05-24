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

#include "CanvasComponentContainer.hpp"
#include "CanvasComponentAllocator.hpp"
#include "../DrawData.hpp"
#include "../DrawingProgram/DrawingProgram.hpp"
#include "../World.hpp"
#include "CanvasComponent.hpp"
#include <Helpers/NetworkingObjects/DelayUpdateSerializedClassManager.hpp>
#include <include/core/SkScalar.h>
#include "../ScaleUpCanvas.hpp"
#include "Helpers/NetworkingObjects/NetObjManager.hpp"

using namespace NetworkingObjects;

void CanvasComponentContainer::CopyData::scale_up(const WorldScalar& scaleUpAmount) {
    coords.scale_about(WorldVec{0, 0}, scaleUpAmount, true);
}

CanvasComponentContainer::CanvasComponentContainer() { }

CanvasComponentContainer::CanvasComponentContainer(NetworkingObjects::NetObjManager& objMan, CanvasComponentType type) {
    compAllocator = objMan.make_obj_direct<CanvasComponentAllocator>(type);
    compAllocator->comp->compContainer = this;
}

CanvasComponentContainer::CanvasComponentContainer(NetworkingObjects::NetObjManager& objMan, const CopyData& copyData) {
    compAllocator = objMan.make_obj_direct<CanvasComponentAllocator>(copyData.obj->get_type());
    compAllocator->comp->compContainer = this;
    compAllocator->comp->set_data_from(*copyData.obj);
    coords = copyData.coords;
}

void CanvasComponentContainer::register_class(World& w) {
    auto readConstructorFunc = [&w](const NetworkingObjects::NetObjTemporaryPtr<CanvasComponentContainer>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) {
        a(o->coords);
        o->compAllocator = o.get_obj_man()->read_create_message<CanvasComponentAllocator>(a, c);
        o->compAllocator->comp->compContainer = o.get();
        canvas_scale_up_check(*o, w, c);
    };
    w.netObjMan.register_class<CanvasComponentContainer, CanvasComponentContainer, CanvasComponentContainer, CanvasComponentContainer>({
        .writeConstructorFuncClient = write_constructor_func,
        .readConstructorFuncClient = readConstructorFunc,
        .readUpdateFuncClient = nullptr,
        .writeConstructorFuncServer = write_constructor_func,
        .readConstructorFuncServer = readConstructorFunc,
        .readUpdateFuncServer = nullptr,
    });
}

void CanvasComponentContainer::reassign_netobj_ids_call() {
    compAllocator.reassign_ids();
}

std::unique_ptr<CanvasComponentContainer::CopyData> CanvasComponentContainer::get_data_copy() const {
    auto toRet = std::make_unique<CanvasComponentContainer::CopyData>();
    toRet->coords = coords;
    toRet->obj = compAllocator->comp->get_data_copy();
    return toRet;
}

void CanvasComponentContainer::send_comp_update(DrawingProgram& drawP, bool finalUpdate) {
    drawP.world.delayedUpdateObjectManager.send_update_to_all<CanvasComponentAllocator>(compAllocator, finalUpdate);
}

void CanvasComponentContainer::write_constructor_func(const NetworkingObjects::NetObjTemporaryPtr<CanvasComponentContainer>& o, cereal::PortableBinaryOutputArchive& a) {
    a(o->coords);
    o->compAllocator.write_create_message(a);
}

void CanvasComponentContainer::save_file(cereal::PortableBinaryOutputArchive& a) const {
    a(coords);
    compAllocator->save_file(a);
}

void CanvasComponentContainer::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version, NetObjManager& objMan) {
    if(version >= VersionNumber(0, 4, 0)) {
        a(coords);
        compAllocator = objMan.make_obj_direct<CanvasComponentAllocator>();
        compAllocator->load_file(a, version);
        compAllocator->comp->compContainer = this;
    }
    else {
        CanvasComponentType t;
        NetworkingObjects::NetObjID uselessID;
        a(t, uselessID, coords);
        compAllocator = objMan.make_obj_direct<CanvasComponentAllocator>(t);
        compAllocator->comp->load_file(a, version);
        compAllocator->comp->compContainer = this;
    }
}

void CanvasComponentContainer::draw(SkCanvas* canvas, const DrawData& drawData) const {
    if(should_draw(drawData))
        draw_with_predraw_data(canvas, drawData, calculate_predraw_data(drawData));
}

void CanvasComponentContainer::draw_with_predraw_data(SkCanvas* canvas, const DrawData& drawData, const PreDrawData& preDrawData) const {
    if(preDrawData.transformData.scale < COMP_MAX_BEFORE_STOP_SCALING || !get_comp().accurate_draw(canvas, drawData, coords, preDrawData.extraData)) {
        canvas->save();
        canvas_do_transform(canvas, preDrawData.transformData);
        get_comp().draw(canvas, drawData, preDrawData.extraData);
        canvas->restore();
    }
}

void CanvasComponentContainer::commit_update(DrawingProgram& drawP) {
    drawP.preupdate_component(&(*objInfo));
    get_comp().initialize_draw_data(drawP);
    calculate_world_bounds();
    drawP.preupdate_component(&(*objInfo));
}

void CanvasComponentContainer::commit_transform(DrawingProgram& drawP) {
    drawP.preupdate_component(&(*objInfo));
    calculate_world_bounds();
    drawP.preupdate_component(&(*objInfo));
}

void CanvasComponentContainer::commit_update_dont_invalidate_cache(DrawingProgram& drawP) {
    get_comp().initialize_draw_data(drawP);
    calculate_world_bounds();
}

void CanvasComponentContainer::commit_transform_dont_invalidate_cache() {
    calculate_world_bounds();
}

// We could just send one of the checkAgainst colliders, since they represent the same thing, but sending both saves on redundant transformations, since we can save both of the versions on the executor side
bool CanvasComponentContainer::collides_with(const CoordSpaceHelper& camCoords, const SkPath& checkAgainstCam) const {
    if(!worldAABB.has_value())
        return false;
    if((camCoords.inverseScale << COMP_MAX_SHIFT_BEFORE_STOP_COLLISIONS) < coords.inverseScale) // Object is too large, just dismiss the collision
        return false;
    else if(coords.inverseScale < (camCoords.inverseScale >> COMP_COLLIDE_MIN_SHIFT_TINY))
        return checkAgainstCam.contains(convert_vec2<SkPoint>(camCoords.to_space(worldAABB.value().min)));
    else {
        TransformData drawTransform = calculate_draw_transform(camCoords);
        SkMatrix m = SkMatrix::I();
        m.postScale(1.0 / drawTransform.scale, 1.0 / drawTransform.scale).postRotate(-drawTransform.rotation).postTranslate(-drawTransform.translation.x(), -drawTransform.translation.y());
        std::optional<SkPath> transformedPath = checkAgainstCam.tryMakeTransform(m);
        if(!transformedPath.has_value())
            return false;
        return get_comp().collides_within_coords_skpath(transformedPath.value());
    }
}

bool CanvasComponentContainer::collides_with_point(const CoordSpaceHelper& camCoords, const Vector2f& checkAgainstCam) const {
    if(!worldAABB.has_value())
        return false;
    if((camCoords.inverseScale << COMP_MAX_SHIFT_BEFORE_STOP_COLLISIONS) < coords.inverseScale) // Object is too large, just dismiss the collision
        return false;
    else if(coords.inverseScale < (camCoords.inverseScale >> COMP_COLLIDE_MIN_SHIFT_TINY)) // Object is too tiny, just dismiss the collision
        return false;
    else {
        TransformData drawTransform = calculate_draw_transform(camCoords);
        SkMatrix m = SkMatrix::I();
        m.postScale(1.0 / drawTransform.scale, 1.0 / drawTransform.scale).postRotate(-drawTransform.rotation).postTranslate(-drawTransform.translation.x(), -drawTransform.translation.y());
        SkPoint p = m.mapPoint(convert_vec2<SkPoint>(checkAgainstCam));
        return get_comp().collides_within_coords_point(convert_vec2<Vector2f>(p));
    }
}

void CanvasComponentContainer::canvas_do_transform(SkCanvas* canvas, const TransformData& transformData) const {
    canvas->scale(transformData.scale, transformData.scale);
    canvas->rotate(transformData.rotation);
    canvas->translate(transformData.translation.x(), transformData.translation.y());
}

bool CanvasComponentContainer::should_draw(const DrawData& drawData) const {
    return (!drawData.clampDrawBetween || (drawData.clampDrawMinimum < coords.inverseScale)) && worldAABB.has_value() && SCollision::collide(worldAABB.value(), drawData.cam.viewingAreaGenerousCollider) && get_comp().should_draw_extra(drawData, coords);
}

CanvasComponentContainer::PreDrawData CanvasComponentContainer::calculate_predraw_data(const DrawData& drawData) const {
    PreDrawData toRet;
    toRet.transformData = calculate_draw_transform(drawData.cam.c);
    if(toRet.transformData.scale < COMP_MAX_BEFORE_STOP_SCALING)
        toRet.extraData = get_comp().get_predraw_data(drawData);
    else
        toRet.extraData = get_comp().get_predraw_data_accurate(drawData, coords);
    return toRet;
}

CanvasComponentContainer::TransformData CanvasComponentContainer::calculate_draw_transform(const CoordSpaceHelper& camCoords) const {
    TransformData toRet;
    toRet.translation = -coords.to_space(camCoords.pos);
    toRet.rotation = (coords.rotation - camCoords.rotation) * 180.0 / std::numbers::pi;
    toRet.scale = std::min(static_cast<float>(coords.inverseScale / camCoords.inverseScale), COMP_MAX_BEFORE_STOP_SCALING);
    return toRet;
}

void CanvasComponentContainer::scale_up(const WorldScalar& scaleUpAmount) {
    coords.scale_about(WorldVec{0, 0}, scaleUpAmount, true);
    calculate_world_bounds();
}

unsigned CanvasComponentContainer::get_mipmap_level(const DrawData& drawData) const {
    if(drawData.mipMapLevelTwo > coords.inverseScale)
        return 2;
    else if(drawData.mipMapLevelOne > coords.inverseScale)
        return 1;
    else
        return 0;
}

void CanvasComponentContainer::calculate_world_bounds() {
    worldAABB = coords.collider_to_world<SCollision::AABB<WorldScalar>, SCollision::AABB<float>>(get_comp().get_obj_coord_bounds());
    WorldVec dim = worldAABB.value().dim();
    if(dim.x() == WorldScalar(0) || dim.y() == WorldScalar(0))
        worldAABB = std::nullopt;
}

const std::optional<SCollision::AABB<WorldScalar>>& CanvasComponentContainer::get_world_bounds() const {
    return worldAABB;
}

CanvasComponent& CanvasComponentContainer::get_comp() const {
    return *compAllocator->comp;
}
