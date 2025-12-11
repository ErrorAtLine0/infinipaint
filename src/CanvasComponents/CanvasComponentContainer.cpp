#include "CanvasComponentContainer.hpp"

CanvasComponentContainer::CanvasComponentContainer() { }

CanvasComponentContainer::CanvasComponentContainer(CanvasComponent::CompType type) {
    allocate_comp(type);
}

void CanvasComponentContainer::register_class(NetworkingObjects::NetObjManagerTypeList& t) {
    t.register_class<CanvasComponentContainer, CanvasComponentContainer, CanvasComponentContainer, CanvasComponentContainer>({
    });
}

std::unique_ptr<CanvasComponent> CanvasComponentContainer::allocate_comp(CanvasComponent::CompType type) {
    switch(type) {
        case CanvasComponent::CompType::BRUSHSTROKE:
            return std::make_unique<CanvasComponentBrushStroke>();
        case CanvasComponent::CompType::RECTANGLE:
            return std::make_unique<CanvasComponentRectangle>();
        case CanvasComponent::CompType::ELLIPSE:
            return std::make_unique<CanvasComponentEllipse>();
        case CanvasComponent::CompType::TEXTBOX:
            return std::make_unique<CanvasComponentTextBox>();
        case CanvasComponent::CompType::IMAGE:
            return std::make_unique<CanvasComponentImage>();
    }
}

void CanvasComponentContainer::save_file(cereal::PortableBinaryOutputArchive& a) const {
}

void CanvasComponentContainer::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
}

const std::unique_ptr<CanvasComponent>& CanvasComponentContainer::get_comp() {
    return comp;
}

const CoordSpaceHelper& CanvasComponentContainer::get_coords() {
    return coords;
}
