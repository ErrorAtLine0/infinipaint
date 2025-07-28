#include "DrawComponent.hpp"
#include "DrawBrushStroke.hpp"
#include "DrawEllipse.hpp"
#include "DrawRectangle.hpp"
#include "DrawTextBox.hpp"
#include "DrawImage.hpp"
#include "Helpers/Networking/NetLibrary.hpp"
#include "../SharedTypes.hpp"
#include "../Server/CommandList.hpp"
#include <chrono>
#include <cereal/types/unordered_set.hpp>

#ifndef IS_SERVER
#include "../MainProgram.hpp"
#endif

void save(cereal::PortableBinaryOutputArchive& a, const CollabListType::ObjectInfo& o) {
    // Client will send ID, but server should ignore ID and generate its own
    a(o.pos, o.obj->get_type(), o.id, o.obj->coords, *o.obj);
}

void load(cereal::PortableBinaryInputArchive& a, CollabListType::ObjectInfo& o) {
    DrawComponentType t;
    a(o.pos, t, o.id);
    o.obj = DrawComponent::allocate_comp_type(t);
    a(o.obj->coords, *o.obj);
}

void SendOrderedComponentVectorOp::save(cereal::PortableBinaryOutputArchive& a) const {
    a((uint64_t)v->size());
    for(auto& c : *v)
        a(*c);
}

void SendOrderedComponentVectorOp::load(cereal::PortableBinaryInputArchive& a) {
    v->clear();
    uint64_t vSize;
    a(vSize);
    for(uint64_t i = 0; i < vSize; i++)
        a(*v->emplace_back(std::make_shared<CollabListType::ObjectInfo>()));
}

std::shared_ptr<DrawComponent> DrawComponent::allocate_comp_type(DrawComponentType type) {
    switch(type) {
        case DRAWCOMPONENT_BRUSHSTROKE:
            return std::make_shared<DrawBrushStroke>();
        case DRAWCOMPONENT_RECTANGLE:
            return std::make_shared<DrawRectangle>();
        case DRAWCOMPONENT_ELLIPSE:
            return std::make_shared<DrawEllipse>();
        case DRAWCOMPONENT_TEXTBOX:
            return std::make_shared<DrawTextBox>();
        case DRAWCOMPONENT_IMAGE:
            return std::make_shared<DrawImage>();
    }
    return nullptr;
}

void DrawComponent::server_send_place_many(MainServer& server, std::vector<CollabListType::ObjectInfoPtr>& comps) {
    server.netServer->send_items_to_all_clients(RELIABLE_COMMAND_CHANNEL, CLIENT_PLACE_MANY_COMPONENTS, SendOrderedComponentVectorOp{&comps});
}

void DrawComponent::server_send_place(MainServer& server, ServerClientID id, uint64_t placement) {
    server.netServer->send_items_to_all_clients(RELIABLE_COMMAND_CHANNEL, CLIENT_PLACE_SINGLE_COMPONENT, placement, get_type(), id, this->coords, *this);
}

void DrawComponent::server_send_erase(MainServer& server, ServerClientID id) {
    server.netServer->send_items_to_all_clients(RELIABLE_COMMAND_CHANNEL, CLIENT_ERASE_SINGLE_COMPONENT, id);
}

void DrawComponent::server_send_erase_set(MainServer& server, const std::unordered_set<ServerClientID>& ids) {
    server.netServer->send_items_to_all_clients(RELIABLE_COMMAND_CHANNEL, CLIENT_ERASE_MANY_COMPONENTS, ids);
}

void DrawComponent::server_send_update_temp(MainServer& server, ServerClientID id) {
    server.netServer->send_items_to_all_clients(UNRELIABLE_COMMAND_CHANNEL, CLIENT_UPDATE_COMPONENT, true, id, *this);
    tempServerUpdateTimer = std::chrono::steady_clock::now();
    serverIsTempUpdate = true;
}

void DrawComponent::server_send_update_final(MainServer& server, ServerClientID id) {
    server.netServer->send_items_to_all_clients(RELIABLE_COMMAND_CHANNEL, CLIENT_UPDATE_COMPONENT, false, id, *this);
    serverIsTempUpdate = false;
}

void DrawComponent::server_send_transform_many(MainServer& server, const std::vector<std::pair<ServerClientID, CoordSpaceHelper>>& transforms) {
    server.netServer->send_items_to_all_clients(RELIABLE_COMMAND_CHANNEL, CLIENT_TRANSFORM_MANY_COMPONENTS, transforms);
}

void DrawComponent::server_update(MainServer& server, ServerClientID id) {
    if(serverIsTempUpdate) {
        float timeSince = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - tempServerUpdateTimer).count();
        if(timeSince >= 1.0f) {
            server_send_update_final(server, id);
            serverIsTempUpdate = false;
        }
    }
}

void DrawComponent::get_used_resources(std::unordered_set<ServerClientID>& v) const {
}

void DrawComponent::remap_resource_ids(std::unordered_map<ServerClientID, ServerClientID>& newIDs) {
}

#ifndef IS_SERVER

bool DrawComponent::bounds_draw_check(const DrawData& drawData) const {
    bool a = !drawData.clampDrawBetween || ((drawData.clampDrawMaximum >= coords.inverseScale) && (drawData.clampDrawMinimum < coords.inverseScale));
    return a;
}

void DrawComponent::check_timers(DrawingProgram& drawP) {
    if(delayedUpdatePtr) {
        float updateDur = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - lastUpdateTime).count();
        if(updateDur > CLIENT_DRAWCOMP_DELAY_TIMER_DURATION) {
            update_from_delayed_ptr();
            delayedUpdatePtr = nullptr;
            final_update(drawP);
        }
    }
}

void DrawComponent::temp_update(DrawingProgram& drawP) {
    drawP.compCache.preupdate_component(collabListInfo.lock());
    worldAABB = std::nullopt;
    initialize_draw_data(drawP);
}

void DrawComponent::transform_temp_update(DrawingProgram& drawP) {
    drawP.compCache.preupdate_component(collabListInfo.lock());
    worldAABB = std::nullopt;
}

void DrawComponent::final_update(DrawingProgram& drawP, bool invalidateCache) {
    initialize_draw_data(drawP);
    finalize_update(drawP, invalidateCache);
}

void DrawComponent::client_send_place_many(DrawingProgram& drawP, std::vector<CollabListType::ObjectInfoPtr>& comps) {
    drawP.world.con.client_send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_PLACE_MANY_COMPONENTS, SendOrderedComponentVectorOp{&comps});
}

void DrawComponent::client_send_place(DrawingProgram& drawP) {
    //if(collabListInfo.lock()) If this fails, then there's a real issue
        drawP.world.con.client_send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_PLACE_SINGLE_COMPONENT, collabListInfo.lock()->pos, get_type(), this->coords, *this);
}

void DrawComponent::client_send_update_temp(DrawingProgram& drawP) {
    if(collabListInfo.lock())
        drawP.world.con.client_send_items_to_server(UNRELIABLE_COMMAND_CHANNEL, SERVER_UPDATE_COMPONENT, true, collabListInfo.lock()->id, *this);
}

void DrawComponent::client_send_update_final(DrawingProgram& drawP) {
    if(collabListInfo.lock())
        drawP.world.con.client_send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_UPDATE_COMPONENT, false, collabListInfo.lock()->id, *this);
}

void DrawComponent::client_send_transform_many(DrawingProgram& drawP, const std::vector<std::pair<ServerClientID, CoordSpaceHelper>>& transforms) {
    drawP.world.con.client_send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_TRANSFORM_MANY_COMPONENTS, transforms);
}

void DrawComponent::client_send_erase(DrawingProgram& drawP) {
    if(collabListInfo.lock())
        drawP.world.con.client_send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_ERASE_SINGLE_COMPONENT, collabListInfo.lock()->id);
}

void DrawComponent::client_send_erase_set(DrawingProgram& drawP, const std::unordered_set<ServerClientID>& ids) {
    drawP.world.con.client_send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_ERASE_MANY_COMPONENTS, ids);
}

void DrawComponent::finalize_update(DrawingProgram& drawP, bool invalidateCache) {
    if(invalidateCache && collabListInfo.lock())
        drawP.compCache.preupdate_component(collabListInfo.lock());
    create_collider(drawP.colliderAllocated);
}

bool DrawComponent::collides_with_world_coords(const CoordSpaceHelper& camCoords, const SCollision::ColliderCollection<WorldScalar>& checkAgainstWorld, bool colliderAllocated) {
    return collides_with(camCoords, checkAgainstWorld, camCoords.world_collider_to_coords<SCollision::ColliderCollection<float>>(checkAgainstWorld), colliderAllocated);
}

bool DrawComponent::collides_with_cam_coords(const CoordSpaceHelper& camCoords, const SCollision::ColliderCollection<float>& checkAgainstCam, bool colliderAllocated) {
    return collides_with(camCoords, camCoords.collider_to_world<SCollision::ColliderCollection<WorldScalar>>(checkAgainstCam), checkAgainstCam, colliderAllocated);
}

// We could just send one of the checkAgainst colliders, since they represent the same thing, but sending both saves on redundant transformations, since we can save both of the versions on the executor side
bool DrawComponent::collides_with(const CoordSpaceHelper& camCoords, const SCollision::ColliderCollection<WorldScalar>& checkAgainstWorld, const SCollision::ColliderCollection<float>& checkAgainstCam, bool colliderAllocated) {
    if((camCoords.inverseScale << DRAWCOMP_MAX_SHIFT_BEFORE_DISAPPEAR) < coords.inverseScale) // Object is too large, just dismiss the collision
        return false;
    else if((camCoords.inverseScale >> DRAWCOMP_COLLIDE_MIN_SHIFT_TINY) >= coords.inverseScale) {
        if(!worldAABB)
            return false;
        return SCollision::collide(checkAgainstCam, camCoords.to_space(worldAABB->min));
    }
    else {
        auto c = checkAgainstWorld.transform<float>(
        [&coords = coords](const WorldVec& a) {
            return coords.to_space(a);
        },
        [&coords = coords](const WorldScalar& a) {
            return coords.scalar_to_space(a);
        });
        return collides_within_coords(c, colliderAllocated);
    }
}

void DrawComponent::canvas_do_calculated_transform(SkCanvas* canvas) {
    canvas->scale(drawSetupData.transformData.scale, drawSetupData.transformData.scale);
    canvas->rotate(drawSetupData.transformData.rotation);
    canvas->translate(drawSetupData.transformData.translation.x(), drawSetupData.transformData.translation.y());
}

void DrawComponent::calculate_draw_transform(const DrawData& drawData) {
    drawSetupData.shouldDraw = !drawData.clampDrawBetween || ((drawData.clampDrawMaximum >= coords.inverseScale) && (drawData.clampDrawMinimum < coords.inverseScale));
    if(drawSetupData.shouldDraw) {
        drawSetupData.shouldDraw = !worldAABB || SCollision::collide(*worldAABB, drawData.cam.viewingAreaGenerousCollider);
        if(drawSetupData.shouldDraw) {
            if(drawData.mipMapLevelTwo > coords.inverseScale)
                drawSetupData.mipmapLevel = 2;
            else if(drawData.mipMapLevelOne > coords.inverseScale)
                drawSetupData.mipmapLevel = 1;
            else
                drawSetupData.mipmapLevel = 0;
            drawSetupData.transformData.translation = -coords.to_space(drawData.cam.c.pos);
            drawSetupData.transformData.rotation = (coords.rotation - drawData.cam.c.rotation) * 180.0 / std::numbers::pi;
            drawSetupData.transformData.scale = static_cast<float>(coords.inverseScale / drawData.cam.c.inverseScale);
        }
    }
}

void DrawComponent::calculate_world_bounds() {
    worldAABB = std::optional<SCollision::AABB<WorldScalar>>(coords.collider_to_world<SCollision::AABB<WorldScalar>, SCollision::AABB<float>>(get_obj_coord_bounds()));
}

std::optional<SCollision::AABB<WorldScalar>> DrawComponent::get_world_bounds() const {
    return worldAABB;
}

#endif

DrawComponent::~DrawComponent() { }
