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

#include "EllipseCanvasComponent.hpp"
#include "CanvasComponent.hpp"
#include "Helpers/ConvertVec.hpp"
#include "Helpers/SCollision.hpp"
#include <include/core/SkPathBuilder.h>
#include "../DrawCollision.hpp"
#include <include/pathops/SkPathOps.h>

CanvasComponentType EllipseCanvasComponent::get_type() const {
    return CanvasComponentType::ELLIPSE;
}

void EllipseCanvasComponent::save(cereal::PortableBinaryOutputArchive& a) const {
    a(d.strokeColor, d.fillColor, d.strokeWidth, d.p1, d.p2, d.fillStrokeMode);
}

void EllipseCanvasComponent::load(cereal::PortableBinaryInputArchive& a) {
    a(d.strokeColor, d.fillColor, d.strokeWidth, d.p1, d.p2, d.fillStrokeMode);
}

void EllipseCanvasComponent::save_file(cereal::PortableBinaryOutputArchive& a) const {
    a(d.strokeColor, d.fillColor, d.strokeWidth, d.p1, d.p2, d.fillStrokeMode);
}

void EllipseCanvasComponent::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    a(d.strokeColor, d.fillColor, d.strokeWidth, d.p1, d.p2, d.fillStrokeMode);
}

void EllipseCanvasComponent::change_stroke_color(const Vector4f& newStrokeColor) {
    d.strokeColor = newStrokeColor;
}

std::optional<Vector4f> EllipseCanvasComponent::get_stroke_color() const {
    return d.strokeColor;
}

void EllipseCanvasComponent::draw(SkCanvas* canvas, const DrawData& drawData, const std::shared_ptr<void>& predrawData) const {
    SkPaint p;
    p.setAntiAlias(drawData.skiaAA);
    if(d.fillStrokeMode == 0 || d.fillStrokeMode == 2) {
        p.setStyle(SkPaint::kFill_Style);
        p.setColor4f(convert_vec4<SkColor4f>(d.fillColor));
        canvas->drawPath(ellipsePath, p);
    }
    if(d.fillStrokeMode == 1 || d.fillStrokeMode == 2) {
        p.setStyle(SkPaint::kStroke_Style);
        p.setColor4f(convert_vec4<SkColor4f>(d.strokeColor));
        p.setStrokeWidth(d.strokeWidth);
        canvas->drawPath(ellipsePath, p);
    }
}

void EllipseCanvasComponent::create_draw_data() {
    SkPathBuilder ellipsePathBuilder;
    SkRect newRect = SkRect::MakeLTRB(d.p1.x(), d.p1.y(), d.p2.x(), d.p2.y());
    ellipsePathBuilder.addOval(newRect);
    ellipsePath = ellipsePathBuilder.detach();
}

void EllipseCanvasComponent::initialize_draw_data(DrawingProgram& drawP) {
    create_draw_data();
}

std::unique_ptr<CanvasComponent> EllipseCanvasComponent::get_data_copy() const {
    auto toRet = std::make_unique<EllipseCanvasComponent>();
    toRet->d = d;
    return toRet;
}

void EllipseCanvasComponent::set_data_from(const CanvasComponent& other) {
    auto& otherEllipse = static_cast<const EllipseCanvasComponent&>(other);
    d = otherEllipse.d;
}

bool EllipseCanvasComponent::collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) const {
    return false;
}

bool EllipseCanvasComponent::collides_within_coords_skpath(const SkPath& checkAgainst) const {
    return checkAgainst.getBounds().intersects(ellipsePath.getBounds()) && Op(checkAgainst, ellipsePath, SkPathOp::kIntersect_SkPathOp);
}

SCollision::AABB<float> EllipseCanvasComponent::get_obj_coord_bounds() const {
    return ellipsePath.getBounds();
}
