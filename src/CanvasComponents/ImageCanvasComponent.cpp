#include "ImageCanvasComponent.hpp"
#include "Eigen/Core"
#include "Helpers/ConvertVec.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/SCollision.hpp"
#include "../SharedTypes.hpp"
#include "../World.hpp"
#include "../MainProgram.hpp"
#include <include/core/SkBlendMode.h>
#include <include/core/SkSamplingOptions.h>
#include "../DrawCollision.hpp"
#include "CanvasComponentContainer.hpp"

SkRect get_cropped_rectangle(const Vector2f& p1, const Vector2f& p2, const Vector2f& cropP1, const Vector2f& cropP2) {
    float pWidth = p2.x() - p1.x();
    float pHeight = p2.y() - p1.y();
    return SkRect::MakeLTRB(p1.x() + pWidth * cropP1.x(), p1.y() + pHeight * cropP1.y(), p1.x() + pWidth * cropP2.x(), p1.y() + pHeight * cropP2.y());
}

SkRect rect_from_points(const Vector2f& p1, const Vector2f& p2) {
    return SkRect::MakeLTRB(p1.x(), p1.y(), p2.x(), p2.y());
}

CanvasComponentType ImageCanvasComponent::get_type() const {
    return CanvasComponentType::IMAGE;
}

void ImageCanvasComponent::save(cereal::PortableBinaryOutputArchive& a) const {
    a(d.editing, d.p1, d.p2, d.imageID, d.cropP1, d.cropP2);
}

void ImageCanvasComponent::load(cereal::PortableBinaryInputArchive& a) {
    a(d.editing, d.p1, d.p2, d.imageID, d.cropP1, d.cropP2);
}

void ImageCanvasComponent::save_file(cereal::PortableBinaryOutputArchive& a) const {
    a(d.p1, d.p2, d.imageID, d.cropP1, d.cropP2);
}

void ImageCanvasComponent::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    a(d.p1, d.p2, d.imageID);
    if(version >= VersionNumber(0, 4, 0))
        a(d.cropP1, d.cropP2);
    else {
        d.cropP1 = Vector2f{0.0f, 0.0f};
        d.cropP2 = Vector2f{1.0f, 1.0f};
    }
    d.editing = false;
}

std::unique_ptr<CanvasComponent> ImageCanvasComponent::get_data_copy() const {
    auto toRet = std::make_unique<ImageCanvasComponent>();
    toRet->d = d;
    return toRet;
}

void ImageCanvasComponent::draw_download_progress_bar(SkCanvas* canvas, const DrawData& drawData, float progress) const {
    if(progress == 0.0f)
        return;
    if(compContainer->should_draw(drawData)) {
        canvas->save();
        compContainer->canvas_do_transform(canvas, compContainer->calculate_draw_transform(drawData));
        SkPaint p;
        p.setStroke(true);
        p.setStrokeWidth(10.0f);
        p.setStrokeCap(SkPaint::kRound_Cap);
        p.setColor4f(drawData.main->toolbar.io->theme->fillColor1);
        Vector2f center = (d.p1 + d.p2) * 0.5f;

        const float DOWNLOAD_ARC_RADIUS = 50.0f;
        canvas->drawArc(SkRect::MakeLTRB(center.x() - DOWNLOAD_ARC_RADIUS, center.y() - DOWNLOAD_ARC_RADIUS, center.x() + DOWNLOAD_ARC_RADIUS, center.y() + DOWNLOAD_ARC_RADIUS), 0.0f, progress * 360.0f, false, p);
        canvas->restore();
    }
}

void ImageCanvasComponent::set_data_from(const CanvasComponent& other) {
    auto& otherImage = static_cast<const ImageCanvasComponent&>(other);
    d = otherImage.d;
}

void ImageCanvasComponent::remap_resource_ids(const std::unordered_map<NetworkingObjects::NetObjID, NetworkingObjects::NetObjID>& resourceOldToNewMap) {
    d.imageID = resourceOldToNewMap.at(d.imageID);
}

void ImageCanvasComponent::get_used_resources(std::unordered_set<NetworkingObjects::NetObjID>& resourceSet) const {
    resourceSet.emplace(d.imageID);
}

void ImageCanvasComponent::draw(SkCanvas* canvas, const DrawData& drawData) const {
    ResourceDisplay* display = drawData.rMan->get_display_data(d.imageID);
    if(d.editing) {
        canvas->saveLayerAlphaf(nullptr, 1.0f);
            display->draw(canvas, drawData, rect_from_points(d.p1, d.p2));
            canvas->saveLayerAlphaf(nullptr, 1.0f);
                SkPaint rectPaint;
                rectPaint.setColor4f(SkColor4f{0.0f, 0.0f, 0.0f, 0.5f});
                canvas->drawRect(rect_from_points(d.p1, d.p2), rectPaint);
                SkPaint openingRectPaint;
                openingRectPaint.setColor4f(SkColor4f{0.0f, 0.0f, 0.0f, 0.0f});
                openingRectPaint.setBlendMode(SkBlendMode::kSrc);
                canvas->drawRect(get_cropped_rectangle(d.p1, d.p2, d.cropP1, d.cropP2), openingRectPaint);
            canvas->restore();
        canvas->restore();
    }
    else {
        bool isCropped = d.cropP1 != Vector2f{0.0f, 0.0f} || d.cropP2 != Vector2f{1.0f, 1.0f};
        if(isCropped) {
            canvas->save();
            canvas->clipRect(get_cropped_rectangle(d.p1, d.p2, d.cropP1, d.cropP2));
        }

        if(display) {
            display->draw(canvas, drawData, rect_from_points(d.p1, d.p2));
            canvas->restore();
        }
        else
            canvas->drawRect(rect_from_points(d.p1, d.p2), SkPaint({0.5f, 0.5f, 0.5f, 0.5f}));

        if(isCropped)
            canvas->restore();
    }
}

void ImageCanvasComponent::update(DrawingProgram& drawP) {
    ResourceDisplay* display = drawP.world.drawData.rMan->get_display_data(d.imageID);
    if(display) {
        if(display->update_draw())
            drawP.invalidate_cache_at_component(&(*compContainer->objInfo));
    }
}

void ImageCanvasComponent::initialize_draw_data(DrawingProgram& drawP) {
    create_collider();
}

bool ImageCanvasComponent::collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) const {
    return collisionTree.is_collide(checkAgainst);
}

void ImageCanvasComponent::create_collider() {
    using namespace SCollision;
    ColliderCollection<float> strokeObjects;

    SkRect startingRect;
    if(d.editing)
        startingRect = rect_from_points(d.p1, d.p2);
    else
        startingRect = get_cropped_rectangle(d.p1, d.p2, d.cropP1, d.cropP2);

    std::array<Vector2f, 4> newT = triangle_from_rect_points(convert_vec2<Vector2f>(startingRect.TL()), convert_vec2<Vector2f>(startingRect.BR()));
    strokeObjects.triangle.emplace_back(newT[0], newT[1], newT[2]);
    strokeObjects.triangle.emplace_back(newT[2], newT[3], newT[0]);
    collisionTree.clear();
    collisionTree.calculate_bvh_recursive(strokeObjects);
}

SCollision::AABB<float> ImageCanvasComponent::get_obj_coord_bounds() const {
    return collisionTree.objects.bounds;
}
