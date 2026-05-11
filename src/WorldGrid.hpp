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
#include <include/core/SkCanvas.h>
#include "DrawData.hpp"
#include <include/effects/SkRuntimeEffect.h>
#include <cereal/types/optional.hpp>

class GridManager;

class WorldGrid {
    public:
        template <class Archive> void serialize(Archive& a) {
            a(name, gridType, size, visible, offset, bounds, color, removeDivisionsOutwards, subdivisions, showCoordinates, displayInFront);
        }

        static void register_class(World& w);
        void draw(GridManager& gMan, SkCanvas* canvas, const DrawData& drawData);
        void draw_coordinates(SkCanvas* canvas, const DrawData& drawData, Vector2f& axisOffset);

        std::string name;
        enum class GridType : unsigned {
            CIRCLE_POINTS = 0,
            SQUARE_POINTS,
            SQUARE_LINES,
            HORIZONTAL_LINES
        } gridType = GridType::CIRCLE_POINTS;
        WorldScalar size{50000000};
        bool visible = true;
        WorldVec offset{0, 0};
        std::optional<SCollision::AABB<WorldScalar>> bounds;
        Vector4f color{1.0f, 1.0f, 1.0f, 1.0f};
        bool removeDivisionsOutwards = false;
        uint32_t subdivisions = 1;
        bool showCoordinates = false;

        std::string get_display_name();

        void set_remove_divisions_outwards(bool v);
        void set_subdivisions(uint32_t v);
        void scale_up(const WorldScalar& scaleUpAmount);

        static constexpr unsigned GRID_UNIT_PIXEL_SIZE = 25;

        WorldScalar coordinatesDivWorldSize;
        WorldScalar coordinatesGridCoordDivSize;
        bool coordinatesWillBeDrawn = false;
        bool coordinatesAxisOnBounds = false;
        bool displayInFront = false;
    private:

        struct ShaderData {
            SkColor4f gridColor;
            float mainGridScale;
            Vector2f mainGridClosestPoint;
            float mainGridPointSize;

            float divGridAlphaFrac;
            float divGridScale;
            Vector2f divGridClosestPoint;
            float divGridPointSize;
        };

        void draw_coordinates(SkCanvas* canvas, const DrawData& drawData, WorldScalar divWorldSize, WorldScalar gridCoordDivSize, Vector2f offsetOtherCoordinates);
        static Vector2f get_closest_grid_point(const WorldVec& gridOffset, const WorldScalar& gridSize, const DrawData& drawData);
        static sk_sp<SkShader> get_shader(GridType gType, const ShaderData& shaderData);
        static sk_sp<SkRuntimeEffect> compile_effect_shader_init(const char* shaderName, const char* shaderCode);
        static Vector2f oldWindowSize;
        static sk_sp<SkRuntimeEffect> circlePointEffect;
        static sk_sp<SkRuntimeEffect> squarePointEffect;
        static sk_sp<SkRuntimeEffect> squareLinesEffect;
        static sk_sp<SkRuntimeEffect> horizontalLinesEffect;
};
