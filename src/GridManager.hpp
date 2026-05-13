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
#include "WorldGrid.hpp"
#include "cereal/archives/portable_binary.hpp"
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include <Helpers/NetworkingObjects/NetObjOwnerPtr.hpp>

class World;

class GridManager {
    public:
        GridManager(World& w);
        void server_init_no_file();
        void read_create_message(cereal::PortableBinaryInputArchive& a);
        void set_grid_list_callbacks();

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
        void finalize_grid_modify(const NetworkingObjects::NetObjTemporaryPtr<WorldGrid>& wGrid, const WorldGrid& oldGridData);
        void refresh_gui_data();
        void setup_list_gui(const std::function<void()>& onStartModify);

        struct GridMenuGUIData {
            std::string newName;
            uint32_t selectedGrid = std::numeric_limits<uint32_t>::max();
        } gridMenu;

        NetworkingObjects::NetObjOwnerPtr<NetworkingObjects::NetObjOrderedList<WorldGrid>> grids;
        World& world;
};
