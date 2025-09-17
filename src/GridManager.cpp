#include "GridManager.hpp"
#include "World.hpp"
#include "MainProgram.hpp"

GridManager::GridManager(World& w):
    world(w) {}

void GridManager::init_client_callbacks() {
}

void GridManager::add_grid(const std::string& name) {
    WorldGrid g;
    g.color = color_mul_alpha(convert_vec4<Vector4f>(world.canvasTheme.toolFrontColor), 0.6f);
    g.set_subdivisions(7);
    g.size = world.drawData.cam.c.inverseScale * WorldScalar(WorldGrid::GRID_UNIT_PIXEL_SIZE);
    g.offset = world.drawData.cam.c.pos + world.drawData.cam.c.dir_from_space(world.main.window.size.cast<float>() * 0.5f);
    grids[name] = g;
    changed = true;
}

void GridManager::remove_grid(const std::string& name) {
    grids.erase(name);
    changed = true;
}

const std::vector<std::string>& GridManager::sorted_names() {
    if(changed) {
        sortedNames.clear();
        for(auto& [k, v] : grids)
            sortedNames.emplace_back(k);
        std::sort(sortedNames.begin(), sortedNames.end());
        changed = false;
    }
    return sortedNames;
}

void GridManager::draw(SkCanvas* canvas, const DrawData& drawData) {
    for(auto& [str, g] : grids)
        g.draw(*this, canvas, drawData);
}
