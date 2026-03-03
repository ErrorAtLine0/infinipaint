#include "EyeDropperTool.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
#include "DrawingProgramToolBase.hpp"
#include <include/core/SkAlphaType.h>
#include <include/core/SkCanvas.h>
#include <include/core/SkColor.h>
#include <include/core/SkColorType.h>
#include <include/core/SkRRect.h>
#include <include/core/SkRect.h>
#include <include/core/SkColorSpace.h>
#include <include/core/SkStream.h>
#include <include/core/SkSurface.h>
#include <Helpers/Logger.hpp>

EyeDropperTool::EyeDropperTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{
}

DrawingProgramToolType EyeDropperTool::get_type() {
    return DrawingProgramToolType::EYEDROPPER;
}

void EyeDropperTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    auto& selectingStrokeColor = drawP.world.main.toolConfig.eyeDropper.selectingStrokeColor;
    t.gui.push_id("Color select tool");
    t.gui.text_label_centered("Color Select");
    if(t.gui.radio_button_field("Stroke Color", "Stroke Color", selectingStrokeColor)) selectingStrokeColor = true;
    if(t.gui.radio_button_field("Fill Color", "Fill Color", !selectingStrokeColor)) selectingStrokeColor = false;
    t.gui.pop_id();
}

bool EyeDropperTool::right_click_popup_gui(Vector2f popupPos) {
    Toolbar& t = drawP.world.main.toolbar;
    t.paint_popup(popupPos);
    return true;
}

void EyeDropperTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    auto& toolConfig = drawP.world.main.toolConfig;
    auto& selectingStrokeColor = drawP.world.main.toolConfig.eyeDropper.selectingStrokeColor;

    if(button.button == InputManager::MouseButton::LEFT && button.down && !drawP.selection.is_being_transformed()) {
        auto& surface = drawP.world.main.window.surface;

        int xPos = std::clamp<int>(button.pos.x(), 0, drawP.world.main.window.size.x() - 1);
        int yPos = std::clamp<int>(button.pos.y(), 0, drawP.world.main.window.size.y() - 1);

        Vector4<uint8_t> readData;
        SkImageInfo readDataInfo = SkImageInfo::Make(1, 1, kRGBA_8888_SkColorType, kUnpremul_SkAlphaType);
        #ifdef USE_SKIA_BACKEND_GRAPHITE
            // NOT WORKING IN GRAPHITE YET
            //surface->makeImageSnapshot()->readPixels(drawP.world.main.window.ctx.get(), readDataInfo, (void*)readData.data(), 4, xPos, yPos);
        #elif USE_SKIA_BACKEND_GANESH
            surface->makeImageSnapshot()->readPixels(drawP.world.main.window.ctx.get(), readDataInfo, (void*)readData.data(), 4, xPos, yPos);
        #endif

        if(selectingStrokeColor)
            toolConfig.globalConf.foregroundColor = readData.cast<float>() / 255.0f;
        else
            toolConfig.globalConf.backgroundColor = readData.cast<float>() / 255.0f;
    }
}

void EyeDropperTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
}

void EyeDropperTool::tool_update() {
}

bool EyeDropperTool::prevent_undo_or_redo() {
    return false;
}

void EyeDropperTool::draw(SkCanvas* canvas, const DrawData& drawData) {
}

void EyeDropperTool::switch_tool(DrawingProgramToolType newTool) {
}
