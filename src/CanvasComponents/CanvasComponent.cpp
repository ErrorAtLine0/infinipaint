#include "CanvasComponent.hpp"
#include "BrushStrokeCanvasComponent.hpp"
#include "ImageCanvasComponent.hpp"
#include "EllipseCanvasComponent.hpp"
#include "RectangleCanvasComponent.hpp"
#include "TextBoxCanvasComponent.hpp"

void CanvasComponent::update(DrawingProgram& drawP) {
}

void CanvasComponent::remap_resource_ids(const std::unordered_map<NetworkingObjects::NetObjID, NetworkingObjects::NetObjID>& resourceOldToNewMap) {
}

void CanvasComponent::get_used_resources(std::unordered_set<NetworkingObjects::NetObjID>& resourceSet) const {
}

CanvasComponent* CanvasComponent::allocate_comp(CanvasComponentType type) {
    switch(type) {
        case CanvasComponentType::BRUSHSTROKE:
            return new BrushStrokeCanvasComponent;
        case CanvasComponentType::RECTANGLE:
            return new RectangleCanvasComponent;
        case CanvasComponentType::ELLIPSE:
            return new EllipseCanvasComponent;
        case CanvasComponentType::TEXTBOX:
            return new TextBoxCanvasComponent;
        case CanvasComponentType::IMAGE:
            return new ImageCanvasComponent;
    }
    return nullptr;
}

void CanvasComponent::change_stroke_color(const Vector4f& newStrokeColor) {
}

std::optional<Vector4f> CanvasComponent::get_stroke_color() const {
    return std::nullopt;
}

bool CanvasComponent::accurate_draw(SkCanvas* canvas, const DrawData& drawData, const CoordSpaceHelper& coords, const std::shared_ptr<void>& predrawData) const {
    return false;
}

std::shared_ptr<void> CanvasComponent::get_predraw_data(const DrawData& drawData) const {
    return nullptr;
}

std::shared_ptr<void> CanvasComponent::get_predraw_data_accurate(const DrawData& drawData, const CoordSpaceHelper& coords) const {
    return nullptr;
}

bool CanvasComponent::should_draw_extra(const DrawData& drawData, const CoordSpaceHelper& coords) const {
    return true;
}

CanvasComponent::~CanvasComponent() {}
