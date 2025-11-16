#include "DrawImage.hpp"
#include "Helpers/ConvertVec.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/SCollision.hpp"
#include "../SharedTypes.hpp"
#include "../World.hpp"

#include "../MainProgram.hpp"

#ifndef IS_SERVER
#include <include/core/SkSamplingOptions.h>
#include "../DrawCollision.hpp"
#endif

DrawComponentType DrawImage::get_type() const {
    return DRAWCOMPONENT_IMAGE;
}

void DrawImage::save(cereal::PortableBinaryOutputArchive& a) const {
    a(d.p1, d.p2, d.imageID);
}

void DrawImage::load(cereal::PortableBinaryInputArchive& a) {
    a(d.p1, d.p2, d.imageID);
}

void DrawImage::save_file(cereal::PortableBinaryOutputArchive& a) const {
    a(d.p1, d.p2, d.imageID);
}

void DrawImage::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    a(d.p1, d.p2, d.imageID);
}

void DrawImage::get_used_resources(std::unordered_set<ServerClientID>& v) const {
    v.emplace(d.imageID);
}

void DrawImage::remap_resource_ids(std::unordered_map<ServerClientID, ServerClientID>& newIDs) {
    auto it = newIDs.find(d.imageID);
    if(it != newIDs.end())
        d.imageID = it->second;
}

#ifndef IS_SERVER
std::shared_ptr<DrawComponent> DrawImage::copy(DrawingProgram& drawP) const {
    auto a = std::make_shared<DrawImage>();
    a->d = d;
    a->coords = coords;
    return a;
}

std::shared_ptr<DrawComponent> DrawImage::deep_copy(DrawingProgram& drawP) const {
    auto a = std::make_shared<DrawImage>();
    a->d = d;
    a->coords = coords;
    a->imRect = imRect;
    a->collisionTree = collisionTree;
    return a;
}

void DrawImage::update_from_delayed_ptr(const std::shared_ptr<DrawComponent>& delayedUpdatePtr) {
    std::shared_ptr<DrawImage> newPtr = std::static_pointer_cast<DrawImage>(delayedUpdatePtr);
    d = newPtr->d;
}

void DrawImage::draw_download_progress_bar(SkCanvas* canvas, const DrawData& drawData, float progress) {
    if(progress == 0.0f)
        return;
    calculate_draw_transform(drawData);
    if(drawSetupData.shouldDraw) {
        canvas->save();
        canvas_do_calculated_transform(canvas);
        SkPaint p;
        p.setStroke(true);
        p.setStrokeWidth(10.0f);
        p.setStrokeCap(SkPaint::kRound_Cap);
        p.setColor4f(drawData.main->toolbar.io->theme->fillColor1);
        Vector2f center = convert_vec2<Vector2f>(imRect.center());

        const float DOWNLOAD_ARC_RADIUS = 50.0f;
        canvas->drawArc(SkRect::MakeLTRB(center.x() - DOWNLOAD_ARC_RADIUS, center.y() - DOWNLOAD_ARC_RADIUS, center.x() + DOWNLOAD_ARC_RADIUS, center.y() + DOWNLOAD_ARC_RADIUS), 0.0f, progress * 360.0f, false, p);
        canvas->restore();
        drawSetupData.shouldDraw = false;
    }
}

void DrawImage::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(drawSetupData.shouldDraw) {
        canvas->save();
        canvas_do_calculated_transform(canvas);
        ResourceDisplay* display = drawData.rMan->get_display_data(d.imageID);
        if(display)
            display->draw(canvas, drawData, imRect);
        else
            canvas->drawRect(imRect, SkPaint({0.5f, 0.5f, 0.5f, 0.5f}));
        canvas->restore();
    }
}

void DrawImage::update(DrawingProgram& drawP) {
    ResourceDisplay* display = drawP.world.drawData.rMan->get_display_data(d.imageID);
    if(display) {
        if(display->update_draw() && collabListInfo.lock())
            drawP.invalidate_cache_at_component(collabListInfo.lock());
    }
}

void DrawImage::initialize_draw_data(DrawingProgram& drawP) {
    create_draw_data();
    create_collider();
}

bool DrawImage::collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) {
    return collisionTree.is_collide(checkAgainst);
}

void DrawImage::create_draw_data() {
    imRect = SkRect::MakeLTRB(d.p1.x(), d.p1.y(), d.p2.x(), d.p2.y());
}

void DrawImage::create_collider() {
    using namespace SCollision;
    ColliderCollection<float> strokeObjects;
    std::array<Vector2f, 4> newT = triangle_from_rect_points(d.p1, d.p2);
    strokeObjects.triangle.emplace_back(newT[0], newT[1], newT[2]);
    strokeObjects.triangle.emplace_back(newT[2], newT[3], newT[0]);
    collisionTree.clear();
    collisionTree.calculate_bvh_recursive(strokeObjects);
}

SCollision::AABB<float> DrawImage::get_obj_coord_bounds() const {
    return collisionTree.objects.bounds;
}

#endif
