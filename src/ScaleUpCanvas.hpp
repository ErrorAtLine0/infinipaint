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

#pragma once
#include <Helpers/NetworkingObjects/NetObjManager.hpp>
#include "World.hpp"

// Arbitrary value. Higher scale up values lead to less scale up events (which can cause stuttering), but leads to larger numbers overall, which can cause a tiny bit of lag and increased memory usage
#define CANVAS_SCALE_UP_STEP WorldScalar("100000000000000000000000000000000000000000000000000")

WorldScalar get_canvas_scale_up_amount(uint32_t newGridSize, uint32_t oldGridSize);

template <typename T> void canvas_scale_up_check(T& obj, World& world, const std::shared_ptr<NetServer::ClientData>& c) {
    if(c) { // Server, compare server's grid size with client's (c) grid size
        auto clientData = world.netObjMan.get_obj_temporary_ref_from_id<ClientData>(NetworkingObjects::NetObjID(c->customID));
        if(clientData->get_grid_size() < world.ownClientData->get_grid_size())
            obj.scale_up(get_canvas_scale_up_amount(world.ownClientData->get_grid_size(), clientData->get_grid_size()));
    }
    else { // Client, compare own grid size (world.ownClientData) with server's grid size
        if(world.serverClientData->get_grid_size() < world.ownClientData->get_grid_size())
            obj.scale_up(get_canvas_scale_up_amount(world.ownClientData->get_grid_size(), world.serverClientData->get_grid_size()));
    }
}
