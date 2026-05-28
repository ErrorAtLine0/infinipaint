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
#include <cstdint>
#include <cereal/archives/portable_binary.hpp>
#include <Helpers/VersionNumber.hpp>
#include <include/core/SkCanvas.h>
#include <Helpers/SCollision.hpp>
#include "CanvasComponentType.hpp"
#include "CanvasComponentContainer.hpp"

class DrawingProgram;

class CanvasComponent {
    public:

        virtual CanvasComponentType get_type() const = 0;
        virtual ~CanvasComponent();
        static CanvasComponent* allocate_comp(CanvasComponentType type);
        virtual void save(cereal::PortableBinaryOutputArchive& a) const = 0;
        virtual void load(cereal::PortableBinaryInputArchive& a) = 0;
        virtual void save_file(cereal::PortableBinaryOutputArchive& a) const = 0;
        virtual void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) = 0;
        virtual void update(DrawingProgram& drawP);
        virtual void get_used_resources(std::unordered_set<NetworkingObjects::NetObjID>& resourceSet) const;
        virtual void remap_resource_ids(const std::unordered_map<NetworkingObjects::NetObjID, NetworkingObjects::NetObjID>& resourceOldToNewMap);
        virtual void change_stroke_color(const Vector4f& newStrokeColor);
        virtual std::vector<CanvasComponentContainer*> attempt_split(DrawingProgram& drawP) const;
        virtual void simplify_paths();
        virtual std::optional<Vector4f> get_stroke_color() const;
        virtual void normalize_object_coordinates(CoordSpaceHelper& coords);

        virtual void set_data_from(const CanvasComponent& other) = 0;
        virtual std::unique_ptr<CanvasComponent> get_data_copy() const = 0;
    protected:
        friend class CanvasComponentContainer;
        friend class CanvasComponentAllocator;

        virtual bool accurate_draw(SkCanvas* canvas, const DrawData& drawData, const CoordSpaceHelper& coords, const std::shared_ptr<void>& predrawData) const;
        virtual std::shared_ptr<void> get_predraw_data(const DrawData& drawData) const;
        virtual std::shared_ptr<void> get_predraw_data_accurate(const DrawData& drawData, const CoordSpaceHelper& coords) const;

        virtual bool should_draw_extra(const DrawData& drawData, const CoordSpaceHelper& coords) const;

        virtual void draw(SkCanvas* canvas, const DrawData& drawData, const std::shared_ptr<void>& predrawData) const = 0;
        virtual void initialize_draw_data(DrawingProgram& drawP) = 0;
        virtual bool collides_within_coords_point(const Vector2f& checkAgainst) const = 0;
        virtual bool collides_within_coords_skpath(const SkPath& checkAgainst) const = 0;

        virtual bool can_erase_detail() const;
        virtual CanvasComponentEraseDetailResult erase_detail(const SkPath& eraseAgainst);

        virtual SCollision::AABB<float> get_obj_coord_bounds() const = 0;

        CanvasComponentContainer* compContainer = nullptr;
};
