#include "GridManager.hpp"
#include "Server/CommandList.hpp"
#include "SharedTypes.hpp"
#include "World.hpp"
#include "MainProgram.hpp"
#include <algorithm>

GridManager::GridManager(World& w):
    world(w) {}

void GridManager::init_client_callbacks() {
    world.con.client_add_recv_callback(CLIENT_SET_GRID, [&](cereal::PortableBinaryInputArchive& message) {
        ServerClientID gID;
        message(gID);
        auto& g = grids[gID];
        message(g);
        changed = true;
    });
    world.con.client_add_recv_callback(CLIENT_REMOVE_GRID, [&](cereal::PortableBinaryInputArchive& message) {
        ServerClientID gID;
        message(gID);
        grids.erase(gID);
        changed = true;
    });
}

ServerClientID GridManager::add_default_grid(const std::string& newName) {
    ServerClientID newID = world.get_new_id();
    WorldGrid g;
    g.name = newName;
    g.color = color_mul_alpha(convert_vec4<Vector4f>(world.canvasTheme.toolFrontColor), 0.6f);
    g.size = world.drawData.cam.c.inverseScale * WorldScalar(WorldGrid::GRID_UNIT_PIXEL_SIZE);
    g.offset = world.drawData.cam.c.pos + world.drawData.cam.c.dir_from_space(world.main.window.size.cast<float>() * 0.5f);
    grids[newID] = g;
    changed = true;
    return newID;
}

void GridManager::send_grid_info(ServerClientID gridID) {
    auto gridFoundIt = grids.find(gridID);
    if(gridFoundIt != grids.end()) {
        WorldGrid& g = gridFoundIt->second;
        world.con.client_send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_SET_GRID, gridID, g);
    }
}

void GridManager::remove_grid(ServerClientID idToRemove) {
    grids.erase(idToRemove);
    world.con.client_send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_REMOVE_GRID, idToRemove);
    changed = true;
}

const std::vector<ServerClientID>& GridManager::sorted_grid_ids() {
    if(changed) {
        sortedGridIDs.clear();
        for(auto& [k, v] : grids)
            sortedGridIDs.emplace_back(k);
        std::sort(sortedGridIDs.begin(), sortedGridIDs.end(), [&](ServerClientID a, ServerClientID b) {
            std::string nameA = grids[a].get_display_name();
            std::string nameB = grids[b].get_display_name();
            bool namesEqual = nameA == nameB;
            return (!namesEqual && std::lexicographical_compare(nameA.begin(), nameA.end(), nameB.begin(), nameB.end())) || (namesEqual && a < b);
        });
        changed = false;
    }
    return sortedGridIDs;
}

ClientPortionID GridManager::get_max_id(ServerPortionID serverID) const {
    ClientPortionID maxClientID = 0;
    for(auto& p : grids) {
        if(p.first.first == serverID)
            maxClientID = std::max(maxClientID, p.first.second);
    }
    return maxClientID;
}

void GridManager::draw(SkCanvas* canvas, const DrawData& drawData) {
    for(auto& [str, g] : grids)
        g.draw(*this, canvas, drawData);
}
