#pragma once
#include "WorldGrid.hpp"

class World;

class GridManager {
    public:
        GridManager(World& w);
        void init_client_callbacks();
        void add_grid(const std::string& name);
        void remove_grid(const std::string& name);
        void draw(SkCanvas* canvas, const DrawData& drawData);
        const std::vector<std::string>& sorted_names();

        std::unordered_map<std::string, WorldGrid> grids;
        World& world;
    private:
        bool changed = true; 
        std::vector<std::string> sortedNames;
};
