#pragma once
#include <Helpers/NetworkingObjects/NetObjManager.hpp>
#include "World.hpp"

// Arbitrary value. Higher scale up values lead to less scale up events (which can cause stuttering), but leads to larger numbers overall, which can cause a tiny bit of lag and increased memory usage
#define CANVAS_SCALE_UP_STEP WorldScalar("100000000000000000000000000000000000000000000000000")

WorldScalar get_canvas_scale_up_amount(uint32_t newGridSize, uint32_t oldGridSize);

template <typename T> void canvas_scale_up_check(T& obj, World& world, const std::shared_ptr<NetServer::ClientData>& c) {
    if(c) {
        auto clientData = world.netObjMan.get_obj_temporary_ref_from_id<ClientData>(NetworkingObjects::NetObjID(c->customID));
        if(clientData->get_grid_size() < world.ownClientData->get_grid_size())
            obj.scale_up(get_canvas_scale_up_amount(world.ownClientData->get_grid_size(), clientData->get_grid_size()));
    }
}
