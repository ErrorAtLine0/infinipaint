#pragma once
#include "WorldGrid.hpp"
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include <Helpers/NetworkingObjects/NetObjOwnerPtr.hpp>

class World;

class GridManager {
    public:
        GridManager(World& w);
        void server_init_no_file();

        void add_default_grid(const std::string& newName);

        template <typename Archive> void save(Archive& a) const {
            a(grids->size());
            for(uint32_t i = 0; i < grids->size(); i++)
                a(*grids->at(i));
        }

        template <typename Archive> void load(Archive& a) {
            uint32_t newSize = grids->size();
            for(uint32_t i = 0; i < newSize; i++) {
                WorldGrid b;
                a(b);
                grids->emplace_back_direct(grids, b);
            }
        }

        void remove_grid(uint32_t indexToRemove);
        void draw_back(SkCanvas* canvas, const DrawData& drawData);
        void draw_front(SkCanvas* canvas, const DrawData& drawData);
        void draw_coordinates(SkCanvas* canvas, const DrawData& drawData);
        void scale_up(const WorldScalar& scaleUpAmount);
        void save_file(cereal::PortableBinaryOutputArchive& a) const;
        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version);

        NetworkingObjects::NetObjOwnerPtr<NetworkingObjects::NetObjOrderedList<WorldGrid>> grids;
        World& world;
};
