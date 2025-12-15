#include "CanvasComponent.hpp"
#include "BrushStrokeCanvasComponent.hpp"
#include "ImageCanvasComponent.hpp"
#include "EllipseCanvasComponent.hpp"
#include "RectangleCanvasComponent.hpp"
#include "TextBoxCanvasComponent.hpp"

void CanvasComponent::update(DrawingProgram& drawP) {
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

CanvasComponent::~CanvasComponent() {}
