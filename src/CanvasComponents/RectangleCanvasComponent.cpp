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

#include "RectangleCanvasComponent.hpp"
#include "Helpers/ConvertVec.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/SCollision.hpp"
#include <include/core/SkPaint.h>
#include <include/core/SkPathBuilder.h>
#include <include/pathops/SkPathOps.h>
#include "../SharedTypes.hpp"
#include "../DrawCollision.hpp"

CanvasComponentType RectangleCanvasComponent::get_type() const {
    return CanvasComponentType::RECTANGLE;
}

void RectangleCanvasComponent::save(cereal::PortableBinaryOutputArchive& a) const {
    a(d.strokeColor, d.fillColor, d.cornerRadius, d.strokeWidth, d.p1, d.p2, d.fillStrokeMode);
}

void RectangleCanvasComponent::load(cereal::PortableBinaryInputArchive& a) {
    a(d.strokeColor, d.fillColor, d.cornerRadius, d.strokeWidth, d.p1, d.p2, d.fillStrokeMode);
}

void RectangleCanvasComponent::save_file(cereal::PortableBinaryOutputArchive& a) const {
    a(d.strokeColor, d.fillColor, d.cornerRadius, d.strokeWidth, d.p1, d.p2, d.fillStrokeMode);
}

void RectangleCanvasComponent::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    a(d.strokeColor, d.fillColor, d.cornerRadius, d.strokeWidth, d.p1, d.p2, d.fillStrokeMode);
}

void RectangleCanvasComponent::change_stroke_color(const Vector4f& newStrokeColor) {
    d.strokeColor = newStrokeColor;
}

std::optional<Vector4f> RectangleCanvasComponent::get_stroke_color() const {
    return d.strokeColor;
}

std::unique_ptr<CanvasComponent> RectangleCanvasComponent::get_data_copy() const {
    auto toRet = std::make_unique<RectangleCanvasComponent>();
    toRet->d = d;
    return toRet;
}

void RectangleCanvasComponent::draw(SkCanvas* canvas, const DrawData& drawData, const std::shared_ptr<void>& predrawData) const {
    SkPaint p;
    p.setAntiAlias(drawData.skiaAA);
    if(d.fillStrokeMode == 0 || d.fillStrokeMode == 2) {
        p.setStyle(SkPaint::kFill_Style);
        p.setStrokeCap(SkPaint::kRound_Cap);
        p.setColor4f(convert_vec4<SkColor4f>(d.fillColor));
        canvas->drawPath(rectPath, p);
    }
    if(d.fillStrokeMode == 1 || d.fillStrokeMode == 2) {
        p.setStyle(SkPaint::kStroke_Style);
        p.setStrokeCap(SkPaint::kRound_Cap);
        p.setColor4f(convert_vec4<SkColor4f>(d.strokeColor));
        p.setStrokeWidth(d.strokeWidth);
        canvas->drawPath(rectPath, p);
    }
}

void RectangleCanvasComponent::set_data_from(const CanvasComponent& other) {
    auto& otherRect = static_cast<const RectangleCanvasComponent&>(other);
    d = otherRect.d;
}

void RectangleCanvasComponent::initialize_draw_data(DrawingProgram& drawP) {
    create_draw_data();
}

void RectangleCanvasComponent::create_draw_data() {
    SkPathBuilder rectPathBuilder;
    if(d.p1.x() == d.p2.x() || d.p1.y() == d.p2.y()) {
        rectPathBuilder.moveTo(convert_vec2<SkPoint>(d.p1));
        rectPathBuilder.lineTo(convert_vec2<SkPoint>(d.p2));
        rectPath = rectPathBuilder.detach();
    }
    else {
        SkRRect newRect = SkRRect::MakeRectXY(SkRect::MakeLTRB(d.p1.x(), d.p1.y(), d.p2.x(), d.p2.y()), d.cornerRadius, d.cornerRadius);
        rectPathBuilder.addRRect(newRect);
        rectPath = rectPathBuilder.detach();
    }
}

bool RectangleCanvasComponent::collides_within_coords_point(const Vector2f& checkAgainst) const {
    bool intersectsAABB = rectPath.getBounds().contains(checkAgainst.x(), checkAgainst.y());
    if(!intersectsAABB)
        return false;
    return rectPath.contains(checkAgainst.x(), checkAgainst.y());
}

bool RectangleCanvasComponent::collides_within_coords_skpath(const SkPath& checkAgainst) const {
    bool intersectsAABB = rectPath.getBounds().intersects(checkAgainst.getBounds());
    if(!intersectsAABB)
        return false;
    std::optional<SkPath> pathIntersectCheck = Op(checkAgainst, rectPath, SkPathOp::kIntersect_SkPathOp);
    return pathIntersectCheck.has_value() && !pathIntersectCheck.value().isEmpty();
}

SCollision::AABB<float> RectangleCanvasComponent::get_obj_coord_bounds() const {
    return rectPath.getBounds();
}
