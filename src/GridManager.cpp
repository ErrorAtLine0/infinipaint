#include "GridManager.hpp"

GridManager::GridManager(World& w):
    world(w) {}

void GridManager::init_client_callbacks() {
}

void GridManager::add_grid(const std::string& name) {
    WorldGrid g;
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
