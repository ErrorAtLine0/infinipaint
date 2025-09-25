#pragma once
#include "WorldGrid.hpp"

class World;

class GridManager {
    public:
        GridManager(World& w);
        void init_client_callbacks();

        ServerClientID add_default_grid(const std::string& newName);

        template <typename Archive> void save(Archive& a) const {
            a(grids);
        }

        template <typename Archive> void load(Archive& a) {
            a(grids);
            changed = true;
        }

        void send_grid_info(ServerClientID gridID);
        void remove_grid(ServerClientID idToRemove);
        void draw(SkCanvas* canvas, const DrawData& drawData);
        void draw_coordinates(SkCanvas* canvas, const DrawData& drawData);
        ClientPortionID get_max_id(ServerPortionID serverID) const;

        const std::vector<ServerClientID>& sorted_grid_ids();

        std::unordered_map<ServerClientID, WorldGrid> grids;
        World& world;
    private:
        bool changed = true; 
        std::vector<ServerClientID> sortedGridIDs;
};
