#include "CanvasComponent.hpp"
#include "BrushStrokeCanvasComponent.hpp"
#include "ImageCanvasComponent.hpp"
#include "EllipseCanvasComponent.hpp"
#include "RectangleCanvasComponent.hpp"
#include "TextBoxCanvasComponent.hpp"

void CanvasComponent::update(DrawingProgram& drawP) {
}

CanvasComponent* CanvasComponent::allocate_comp(CanvasComponent::CompType type) {
    switch(type) {
        case CanvasComponent::CompType::BRUSHSTROKE:
            return new BrushStrokeCanvasComponent;
        case CanvasComponent::CompType::RECTANGLE:
            return new RectangleCanvasComponent;
        case CanvasComponent::CompType::ELLIPSE:
            return new EllipseCanvasComponent;
        case CanvasComponent::CompType::TEXTBOX:
            return new TextBoxCanvasComponent;
        case CanvasComponent::CompType::IMAGE:
            return new ImageCanvasComponent;
    }
    return nullptr;
}

CanvasComponent::~CanvasComponent() {}
