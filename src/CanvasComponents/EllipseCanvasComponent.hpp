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
#include "CanvasComponent.hpp"
#include "../CoordSpaceHelper.hpp"
#include <include/core/SkPath.h>

class EllipseCanvasComponent : public CanvasComponent {
    public:
        virtual CanvasComponentType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void save_file(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) override;
        virtual void change_stroke_color(const Vector4f& newStrokeColor) override;
        virtual std::optional<Vector4f> get_stroke_color() const override;
        virtual std::unique_ptr<CanvasComponent> get_data_copy() const override;
        virtual void set_data_from(const CanvasComponent& other) override;

        // User input data
        struct Data {
            Vector4f strokeColor;
            Vector4f fillColor;
            float strokeWidth;
            Vector2f p1;
            Vector2f p2;
            uint8_t fillStrokeMode;
            bool operator==(const Data& o) const = default;
        } d;

    private:
        void create_draw_data();
        virtual void draw(SkCanvas* canvas, const DrawData& drawData, const std::shared_ptr<void>& predrawData) const override;
        virtual void initialize_draw_data(DrawingProgram& drawP) override;
        virtual bool collides_within_coords_point(const Vector2f& checkAgainst) const override;
        virtual bool collides_within_coords_skpath(const SkPath& checkAgainst) const override;
        virtual SCollision::AABB<float> get_obj_coord_bounds() const override;
        void create_collider();
        void create_triangles(const std::function<bool(Vector2f, Vector2f, Vector2f)>& passTriangleFunc);
        SkPath colliderPath;
        SkPath ellipsePath;
};
