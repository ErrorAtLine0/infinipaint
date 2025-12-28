#include "CanvasComponentContainer.hpp"
#include "CanvasComponentAllocator.hpp"
#include "../DrawData.hpp"
#include "../DrawingProgram/DrawingProgram.hpp"
#include "../World.hpp"
#include "CanvasComponent.hpp"
#include <Helpers/NetworkingObjects/DelayUpdateSerializedClassManager.hpp>

using namespace NetworkingObjects;

CanvasComponentContainer::CanvasComponentContainer() { }

CanvasComponentContainer::CanvasComponentContainer(NetworkingObjects::NetObjManager& objMan, CanvasComponentType type) {
    compAllocator = objMan.make_obj_direct<CanvasComponentAllocator>(type);
    compAllocator->comp->compContainer = this;
}

void CanvasComponentContainer::register_class(NetObjManager& objMan) {
    objMan.register_class<CanvasComponentContainer, CanvasComponentContainer, CanvasComponentContainer, CanvasComponentContainer>({
        .writeConstructorFuncClient = write_constructor_func,
        .readConstructorFuncClient = read_constructor_func,
        .readUpdateFuncClient = nullptr,
        .writeConstructorFuncServer = write_constructor_func,
        .readConstructorFuncServer = read_constructor_func,
        .readUpdateFuncServer = nullptr,
    });
}

void CanvasComponentContainer::send_comp_update(DrawingProgram& drawP, bool finalUpdate) {
    drawP.world.delayedUpdateObjectManager.send_update_to_all<CanvasComponentAllocator>(compAllocator, finalUpdate);
}

void CanvasComponentContainer::write_constructor_func(const NetworkingObjects::NetObjTemporaryPtr<CanvasComponentContainer>& o, cereal::PortableBinaryOutputArchive& a) {
    a(o->coords);
    o->compAllocator.write_create_message(a);
}

void CanvasComponentContainer::read_constructor_func(const NetworkingObjects::NetObjTemporaryPtr<CanvasComponentContainer>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) {
    a(o->coords);
    o->compAllocator = o.get_obj_man()->read_create_message<CanvasComponentAllocator>(a, c);
    o->compAllocator->comp->compContainer = o.get();
}

void CanvasComponentContainer::save_file(cereal::PortableBinaryOutputArchive& a) const {
    a(coords);
    get_comp().save_file(a);
}

void CanvasComponentContainer::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    a(coords);
    get_comp().load_file(a, version);
}

void CanvasComponentContainer::draw(SkCanvas* canvas, const DrawData& drawData) const {
    if(should_draw(drawData)) {
        canvas->save();
        canvas_do_transform(canvas, calculate_draw_transform(drawData));
        get_comp().draw(canvas, drawData);
        canvas->restore();
    }
}

void CanvasComponentContainer::set_owner_obj_info(const ObjInfoSharedPtr& ownerObjInfo) {
    objInfo = ownerObjInfo;
}

void CanvasComponentContainer::commit_update(DrawingProgram& drawP) {
    auto lockedObjInfo = objInfo.lock();
    if(worldAABB.has_value()) // This is the only point where worldAABB could be nullopt
        drawP.drawCache.preupdate_component(lockedObjInfo);
    get_comp().initialize_draw_data(drawP);
    calculate_world_bounds();
    drawP.drawCache.preupdate_component(lockedObjInfo);
}

void CanvasComponentContainer::commit_transform(DrawingProgram& drawP) {
    auto lockedObjInfo = objInfo.lock();
    if(worldAABB.has_value()) // This is the only point where worldAABB could be nullopt
        drawP.drawCache.preupdate_component(lockedObjInfo);
    calculate_world_bounds();
    drawP.drawCache.preupdate_component(lockedObjInfo);
}

void CanvasComponentContainer::commit_update_dont_invalidate_cache(DrawingProgram& drawP) {
    get_comp().initialize_draw_data(drawP);
    calculate_world_bounds();
}

void CanvasComponentContainer::commit_transform_dont_invalidate_cache(DrawingProgram& drawP) {
    calculate_world_bounds();
}

bool CanvasComponentContainer::collides_with_world_coords(const CoordSpaceHelper& camCoords, const SCollision::ColliderCollection<WorldScalar>& checkAgainstWorld) const {
    return collides_with(camCoords, checkAgainstWorld, camCoords.world_collider_to_coords<SCollision::ColliderCollection<float>>(checkAgainstWorld));
}

bool CanvasComponentContainer::collides_with_cam_coords(const CoordSpaceHelper& camCoords, const SCollision::ColliderCollection<float>& checkAgainstCam) const {
    return collides_with(camCoords, camCoords.collider_to_world<SCollision::ColliderCollection<WorldScalar>>(checkAgainstCam), checkAgainstCam);
}

// We could just send one of the checkAgainst colliders, since they represent the same thing, but sending both saves on redundant transformations, since we can save both of the versions on the executor side
bool CanvasComponentContainer::collides_with(const CoordSpaceHelper& camCoords, const SCollision::ColliderCollection<WorldScalar>& checkAgainstWorld, const SCollision::ColliderCollection<float>& checkAgainstCam) const {
    if((camCoords.inverseScale << COMP_MAX_SHIFT_BEFORE_STOP_COLLISIONS) < coords.inverseScale) // Object is too large, just dismiss the collision
        return false;
    else if((camCoords.inverseScale >> COMP_COLLIDE_MIN_SHIFT_TINY) >= coords.inverseScale)
        return SCollision::collide(checkAgainstCam, camCoords.to_space(worldAABB.value().min));
    else {
        auto c = checkAgainstWorld.transform<float>(
        [&coords = coords](const WorldVec& a) {
            return coords.to_space(a);
        },
        [&coords = coords](const WorldScalar& a) {
            return coords.scalar_to_space(a);
        });
        return get_comp().collides_within_coords(c);
    }
}

void CanvasComponentContainer::canvas_do_transform(SkCanvas* canvas, const TransformDrawData& transformData) const {
    canvas->scale(transformData.scale, transformData.scale);
    canvas->rotate(transformData.rotation);
    canvas->translate(transformData.translation.x(), transformData.translation.y());
}

bool CanvasComponentContainer::should_draw(const DrawData& drawData) const {
    return (!drawData.clampDrawBetween || (drawData.clampDrawMinimum < coords.inverseScale)) && SCollision::collide(worldAABB.value(), drawData.cam.viewingAreaGenerousCollider);
}

CanvasComponentContainer::TransformDrawData CanvasComponentContainer::calculate_draw_transform(const DrawData& drawData) const {
    TransformDrawData toRet;
    toRet.translation = -coords.to_space(drawData.cam.c.pos);
    toRet.rotation = (coords.rotation - drawData.cam.c.rotation) * 180.0 / std::numbers::pi;
    toRet.scale = std::min(static_cast<float>(coords.inverseScale / drawData.cam.c.inverseScale), static_cast<float>(1 << COMP_MAX_SHIFT_BEFORE_STOP_SCALING));
    return toRet;
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
}

SCollision::AABB<WorldScalar> CanvasComponentContainer::get_world_bounds() const {
    return worldAABB.value();
}

CanvasComponent& CanvasComponentContainer::get_comp() const {
    return *compAllocator->comp;
}
