#include "ImageCanvasComponent.hpp"
#include "Helpers/ConvertVec.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/SCollision.hpp"
#include "../SharedTypes.hpp"
#include "../World.hpp"
#include "../MainProgram.hpp"
#include <include/core/SkSamplingOptions.h>
#include "../DrawCollision.hpp"
#include "CanvasComponentContainer.hpp"

CanvasComponentType ImageCanvasComponent::get_type() const {
    return CanvasComponentType::IMAGE;
}

void ImageCanvasComponent::save(cereal::PortableBinaryOutputArchive& a) const {
    a(d.p1, d.p2, d.imageID);
}

void ImageCanvasComponent::load(cereal::PortableBinaryInputArchive& a) {
    a(d.p1, d.p2, d.imageID);
}

void ImageCanvasComponent::save_file(cereal::PortableBinaryOutputArchive& a) const {
    a(d.p1, d.p2, d.imageID);
}

void ImageCanvasComponent::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    a(d.p1, d.p2, d.imageID);
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
        Vector2f center = convert_vec2<Vector2f>(imRect.center());

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
    if(display)
        display->draw(canvas, drawData, imRect);
    else
        canvas->drawRect(imRect, SkPaint({0.5f, 0.5f, 0.5f, 0.5f}));
}

void ImageCanvasComponent::update(DrawingProgram& drawP) {
    ResourceDisplay* display = drawP.world.drawData.rMan->get_display_data(d.imageID);
    if(display) {
        if(display->update_draw())
            drawP.invalidate_cache_at_component(&(*compContainer->objInfo));
    }
}

void ImageCanvasComponent::initialize_draw_data(DrawingProgram& drawP) {
    create_draw_data();
    create_collider();
}

bool ImageCanvasComponent::collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) const {
    return collisionTree.is_collide(checkAgainst);
}

void ImageCanvasComponent::create_draw_data() {
    imRect = SkRect::MakeLTRB(d.p1.x(), d.p1.y(), d.p2.x(), d.p2.y());
}

void ImageCanvasComponent::create_collider() {
    using namespace SCollision;
    ColliderCollection<float> strokeObjects;
    std::array<Vector2f, 4> newT = triangle_from_rect_points(d.p1, d.p2);
    strokeObjects.triangle.emplace_back(newT[0], newT[1], newT[2]);
    strokeObjects.triangle.emplace_back(newT[2], newT[3], newT[0]);
    collisionTree.clear();
    collisionTree.calculate_bvh_recursive(strokeObjects);
}

SCollision::AABB<float> ImageCanvasComponent::get_obj_coord_bounds() const {
    return collisionTree.objects.bounds;
}
