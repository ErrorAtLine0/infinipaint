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
#include "ResourceDisplay.hpp"
#include <include/core/SkImage.h>
#include <vector>
#include <modules/svg/include/SkSVGDOM.h>

class SvgResourceDisplay : public ResourceDisplay {
    public:
        virtual void update(World& w) override;
        virtual bool load(ResourceManager& rMan, const std::string& fileName, const std::shared_ptr<std::string>& fileData) override;
        virtual bool update_draw() const override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData, const SkRect& imRect) override;
        virtual Vector2f get_dimensions() const override;
        virtual float get_dimension_scale() const override;
        virtual Type get_type() const override;
    private:
        sk_sp<SkSVGDOM> svgDom;
        bool mustUpdateDraw = false;
        SkSize svgRootSize;
};
