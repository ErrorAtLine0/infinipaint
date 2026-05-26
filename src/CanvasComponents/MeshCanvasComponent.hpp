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
#include "BrushComponentCode.hpp"
#include "CanvasComponent.hpp"
#include "Helpers/SCollision.hpp"
#include <Helpers/Serializers.hpp>
#include <include/core/SkPath.h>
#include <include/core/SkPathBuilder.h>

class MeshCanvasComponent : public CanvasComponent {
    public:
        virtual CanvasComponentType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void save_file(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) override;
        virtual std::unique_ptr<CanvasComponent> get_data_copy() const override;
        virtual void change_stroke_color(const Vector4f& newStrokeColor) override;
        virtual std::vector<CanvasComponentContainer*> attempt_split(DrawingProgram& drawP) const override;
        virtual std::optional<Vector4f> get_stroke_color() const override;
        virtual void set_data_from(const CanvasComponent& other) override;

        struct Data {
            SkPath meshPath;
            Vector4f color;
        } d;
    private:
        virtual void draw(SkCanvas* canvas, const DrawData& drawData, const std::shared_ptr<void>& predrawData) const override;
        virtual bool accurate_draw(SkCanvas* canvas, const DrawData& drawData, const CoordSpaceHelper& coords, const std::shared_ptr<void>& predrawData) const override;
        virtual std::shared_ptr<void> get_predraw_data_accurate(const DrawData& drawData, const CoordSpaceHelper& coords) const override;
        virtual void initialize_draw_data(DrawingProgram& drawP) override;
        virtual bool collides_within_coords_point(const Vector2f& checkAgainst) const override;
        virtual bool collides_within_coords_skpath(const SkPath& checkAgainst) const override;
        virtual bool can_erase_detail() const override;
        virtual CanvasComponentEraseDetailResult erase_detail(const SkPath& eraseAgainst) override;
        bool should_draw_extra(const DrawData& drawData, const CoordSpaceHelper& coords) const override;
        virtual SCollision::AABB<float> get_obj_coord_bounds() const override;
        SCollision::BVHContainer<float> collisionTree;
};

