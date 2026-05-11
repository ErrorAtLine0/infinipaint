/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/RadioButtonHelpers.hpp"

EyeDropperTool::EyeDropperTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{
}

DrawingProgramToolType EyeDropperTool::get_type() {
    return DrawingProgramToolType::EYEDROPPER;
}

void EyeDropperTool::gui_toolbox(Toolbar& t) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& gui = drawP.world.main.g.gui;
    auto& selectingStrokeColor = drawP.world.main.toolConfig.eyeDropper.selectingStrokeColor;
    gui.new_id("Color select tool", [&] {
        text_label_centered(gui, "Color Select");

        radio_button_selector<bool>(gui, "Stroke type", &selectingStrokeColor, {
            {"Stroke Color", true},
            {"Fill Color", false}
        });
    });
}

void EyeDropperTool::gui_phone_toolbox(PhoneDrawingProgramScreen& t) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& gui = drawP.world.main.g.gui;
    auto& selectingStrokeColor = drawP.world.main.toolConfig.eyeDropper.selectingStrokeColor;
    gui.new_id("Color select tool", [&] {
        radio_button_selector<bool>(gui, "Stroke type", &selectingStrokeColor, {
            {"Stroke Color", true},
            {"Fill Color", false}
        });
    });
}

void EyeDropperTool::right_click_popup_gui(Toolbar& t, Vector2f popupPos) {
    t.paint_popup(popupPos);
}

void EyeDropperTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    auto& toolConfig = drawP.world.main.toolConfig;
    auto& selectingStrokeColor = drawP.world.main.toolConfig.eyeDropper.selectingStrokeColor;

    if(button.button == InputManager::MouseButton::LEFT && button.down && !drawP.selection.is_being_transformed() && !drawP.world.main.g.gui.cursor_obstructed()) {
        auto& surface = drawP.world.main.window.nativeSurface;

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
        drawP.world.main.g.gui.set_to_layout();
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
