#include "DrawingProgramToolBase.hpp"

#include "BrushTool.hpp"
#include "EraserTool.hpp"
#include "RectSelectTool.hpp"
#include "LassoSelectTool.hpp"
#include "RectDrawTool.hpp"
#include "EllipseDrawTool.hpp"
#include "TextBoxTool.hpp"
#include "EyeDropperTool.hpp"
#include "ScreenshotTool.hpp"
#include "EditTool.hpp"
#include "GridModifyTool.hpp"
#include "ZoomCanvasTool.hpp"

DrawingProgramToolBase::DrawingProgramToolBase(DrawingProgram& initDrawP):
    drawP(initDrawP)
{}

DrawingProgramToolBase::~DrawingProgramToolBase() {}

std::unique_ptr<DrawingProgramToolBase> DrawingProgramToolBase::allocate_tool_type(DrawingProgram& drawP, DrawingProgramToolType t) {
    switch(t) {
        case DrawingProgramToolType::BRUSH:
            return std::make_unique<BrushTool>(drawP);
        case DrawingProgramToolType::ERASER:
            return std::make_unique<EraserTool>(drawP);
        case DrawingProgramToolType::LASSOSELECT:
            return std::make_unique<LassoSelectTool>(drawP);
        case DrawingProgramToolType::RECTSELECT:
            return std::make_unique<RectSelectTool>(drawP);
        case DrawingProgramToolType::RECTANGLE:
            return std::make_unique<RectDrawTool>(drawP);
        case DrawingProgramToolType::ELLIPSE:
            return std::make_unique<EllipseDrawTool>(drawP);
        case DrawingProgramToolType::TEXTBOX:
            return std::make_unique<TextBoxTool>(drawP);
        case DrawingProgramToolType::EYEDROPPER:
            return std::make_unique<EyeDropperTool>(drawP);
        case DrawingProgramToolType::SCREENSHOT:
            return std::make_unique<ScreenshotTool>(drawP);
        case DrawingProgramToolType::GRIDMODIFY:
            return std::make_unique<GridModifyTool>(drawP);
        case DrawingProgramToolType::EDIT:
            return std::make_unique<EditTool>(drawP);
        case DrawingProgramToolType::ZOOM:
            return std::make_unique<ZoomCanvasTool>(drawP);
    }
    return nullptr;
}
