#include "DrawingProgramToolBase.hpp"

#include "BrushTool.hpp"
#include "EraserTool.hpp"
#include "PanCanvasTool.hpp"
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
#include "LineDrawTool.hpp"

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
        case DrawingProgramToolType::PAN:
            return std::make_unique<PanCanvasTool>(drawP);
        case DrawingProgramToolType::LINE:
            return std::make_unique<LineDrawTool>(drawP);
    }
    return nullptr;
}

void DrawingProgramToolBase::input_paste_callback(const CustomEvents::PasteEventData& paste) {}
void DrawingProgramToolBase::input_text_key_callback(const InputManager::KeyCallbackArgs& key) {}
void DrawingProgramToolBase::input_text_callback(const InputManager::TextCallbackArgs& text) {}
void DrawingProgramToolBase::input_key_callback(const InputManager::KeyCallbackArgs& key) {}
void DrawingProgramToolBase::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {}
void DrawingProgramToolBase::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {}
void DrawingProgramToolBase::input_pen_button_callback(const InputManager::PenButtonCallbackArgs& button) {}
void DrawingProgramToolBase::input_pen_touch_callback(const InputManager::PenTouchCallbackArgs& touch) {}
void DrawingProgramToolBase::input_pen_motion_callback(const InputManager::PenMotionCallbackArgs& motion) {}
void DrawingProgramToolBase::input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis) {}
