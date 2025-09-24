#include "GridManager.hpp"
#include "SharedTypes.hpp"
#include "World.hpp"
#include "MainProgram.hpp"
#include <algorithm>

GridManager::GridManager(World& w):
    world(w) {}

void GridManager::init_client_callbacks() {
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

void GridManager::remove_grid(ServerClientID idToRemove) {
    grids.erase(idToRemove);
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

void GridManager::draw(SkCanvas* canvas, const DrawData& drawData) {
    for(auto& [str, g] : grids)
        g.draw(*this, canvas, drawData);
}
