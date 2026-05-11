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
#include <include/core/SkRect.h>
#include <Eigen/Dense>
#include "../SharedTypes.hpp"
#include <Helpers/SCollision.hpp>
#include "../CoordSpaceHelper.hpp"

using namespace Eigen;

struct DrawData;
class World;
class ResourceManager;

class ResourceDisplay {
    public:
        enum class Type {
            FILE,
            IMAGE,
            SVG
        };
        virtual void update(World& w) = 0;
        virtual bool load(ResourceManager& rMan, const std::string& fileName, const std::shared_ptr<std::string>& fileData) = 0;
        virtual bool update_draw() const = 0;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData, const SkRect& imRect) = 0;
        virtual Vector2f get_dimensions() const = 0;
        virtual float get_dimension_scale() const = 0;
        virtual Type get_type() const = 0;
        virtual void camera_view_update(const CoordSpaceHelper& compCoords, const SCollision::AABB<WorldScalar>& compAABB, const DrawData& drawData, const SkRect& imRect);
        virtual void clear_cache();
        virtual ~ResourceDisplay();
};
