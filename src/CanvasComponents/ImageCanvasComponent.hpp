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
#include "../SharedTypes.hpp"
#include "CanvasComponent.hpp"
#include "../CoordSpaceHelper.hpp"
#include <include/core/SkPath.h>

class ImageCanvasComponent : public CanvasComponent {
    public:
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void save_file(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) override;
        virtual CanvasComponentType get_type() const override;
        std::unique_ptr<CanvasComponent> get_data_copy() const override;
        virtual void set_data_from(const CanvasComponent& other) override;
        virtual void remap_resource_ids(const std::unordered_map<NetworkingObjects::NetObjID, NetworkingObjects::NetObjID>& resourceOldToNewMap) override;
        virtual void get_used_resources(std::unordered_set<NetworkingObjects::NetObjID>& resourceSet) const override;

        void draw_download_progress_bar(SkCanvas* canvas, const DrawData& drawData, float progress) const;

        // User input data
        struct Data {
            Vector2f p1;
            Vector2f p2;
            Vector2f cropP1 = {0.0f, 0.0f};
            Vector2f cropP2 = {1.0f, 1.0f};

            NetworkingObjects::NetObjID imageID;

            bool editing = false;
        } d;

        virtual void update(DrawingProgram& drawP) override;

        bool contains_actual_image();

    private:
        virtual void draw(SkCanvas* canvas, const DrawData& drawData, const std::shared_ptr<void>& predrawData) const override;
        virtual void initialize_draw_data(DrawingProgram& drawP) override;
        virtual bool collides_within_coords_point(const Vector2f& checkAgainst) const override;
        virtual bool collides_within_coords_skpath(const SkPath& checkAgainst) const override;
        virtual SCollision::AABB<float> get_obj_coord_bounds() const override;
};

