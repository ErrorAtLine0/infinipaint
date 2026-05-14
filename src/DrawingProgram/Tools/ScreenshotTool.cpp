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

#include "ScreenshotTool.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
#include "DrawingProgramToolBase.hpp"
#include "Helpers/SCollision.hpp"
#include <filesystem>
#include <include/core/SkPathBuilder.h>

#include "../../GridManager.hpp"
#include <Helpers/Logger.hpp>

#include "../../MainProgram.hpp"


#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/CheckBoxHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/TextBoxHelpers.hpp"
#include "../../GUIStuff/Elements/DropDown.hpp"

#define SECTION_SIZE 4000
#define AA_LEVEL 4

ScreenshotTool::ScreenshotTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType ScreenshotTool::get_type() {
    return DrawingProgramToolType::SCREENSHOT;
}

void ScreenshotTool::gui_toolbox(Toolbar& t) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& gui = drawP.world.main.g.gui;
    auto& screenshotConfig = drawP.world.main.toolConfig.screenshot;
    gui.new_id("screenshot tool", [&] {
        text_label_centered(gui, "Screenshot");
        if(controls.selectionMode == ScreenshotControls::SelectionMode::NO_SELECTION)
            text_label(gui, "Select an area on the canvas...");
        if(controls.selectionMode != ScreenshotControls::SelectionMode::NO_SELECTION && screenshotConfig.selectedType != WorldScreenshotInfo::ScreenshotType::SVG) {
            input_scalars_field(gui, "Image Size", "Image Size", &controls.imageSize, 2, 0, 999999999, {
                .onEdit = [&] (size_t i) {
                    if(i == 0) {
                        screenshotConfig.setDimensionSize = controls.imageSize.x();
                        screenshotConfig.setDimensionIsX = true;
                        controls.imageSize.y() = controls.imageSize.x() * (controls.rectY2 - controls.rectY1) / (controls.rectX2 - controls.rectX1);
                    }
                    else {
                        screenshotConfig.setDimensionSize = controls.imageSize.y();
                        screenshotConfig.setDimensionIsX = false;
                        controls.imageSize.x() = controls.imageSize.y() * (controls.rectX2 - controls.rectX1) / (controls.rectY2 - controls.rectY1);
                    }
                    gui.set_to_layout();
                }
            });
        }
        if(controls.selectionMode == ScreenshotControls::SelectionMode::SELECTION_EXISTS) {
            left_to_right_line_layout(gui, [&]() {
                text_label(gui, "Image Type");
                gui.element<DropDown<size_t>>("image type select", (size_t*)(&screenshotConfig.selectedType), controls.typeSelections);
            });
            if(screenshotConfig.selectedType != WorldScreenshotInfo::ScreenshotType::SVG)
                checkbox_boolean_field(gui, "Display Grid", "Display Grid", &controls.displayGrid);
            else
                text_label(gui, "Note: Screenshot will ignore blend\nmodes and layer alpha");
            if(screenshotConfig.selectedType != WorldScreenshotInfo::ScreenshotType::JPG)
                checkbox_boolean_field(gui, "Transparent Background", "Transparent Background", &controls.transparentBackground);
            text_button(gui, "Take Screenshot", "Take Screenshot", {
                .wide = true,
                .onClick = [&] {
                    controls.selectionMode = ScreenshotControls::SelectionMode::NO_SELECTION;
                    #ifdef __EMSCRIPTEN__
                        take_screenshot("a" + world_screenshot_info_get_extension_from_type(screenshotConfig.selectedType), screenshotConfig.selectedType);
                    #else
                        // We can't actually use the extension from the callback, so we have to set the extension of choice beforehand
                        Screen::ExtensionFilter setExtensionFilter;
                        switch(screenshotConfig.selectedType) {
                            case WorldScreenshotInfo::ScreenshotType::JPG:
                                setExtensionFilter = {"JPEG", "jpg;jpeg"};
                                break;
                            case WorldScreenshotInfo::ScreenshotType::PNG:
                                setExtensionFilter = {"PNG", "png"};
                                break;
                            case WorldScreenshotInfo::ScreenshotType::WEBP:
                                setExtensionFilter = {"WEBP", "webp"};
                                break;
                            case WorldScreenshotInfo::ScreenshotType::SVG:
                                setExtensionFilter = {"SVG", "svg"};
                                break;
                        }
                        t.open_file_selector("Export", {setExtensionFilter}, [setExtensionFilter, w = make_weak_ptr(drawP.world.main.world)](const std::filesystem::path& p, const auto& e) {
                            auto world = w.lock();
                            if(world && world->drawProg.drawTool->get_type() == DrawingProgramToolType::SCREENSHOT) {
                                ScreenshotTool* screenshotTool = static_cast<ScreenshotTool*>(world->drawProg.drawTool.get());
                                screenshotTool->controls.screenshotSavePath = p;
                                screenshotTool->controls.screenshotSaveType = world->main.toolConfig.screenshot.selectedType;
                                screenshotTool->controls.setToTakeScreenshot = true;
                            }
                        }, "screenshot", true);
                    #endif
                }
            });
        }
    });
}

void ScreenshotTool::gui_phone_toolbox(PhoneDrawingProgramScreen& t) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& gui = drawP.world.main.g.gui;
    gui.new_id("screenshot tool", [&] {
        text_label_centered(gui, "Screenshot");
    });
}

void ScreenshotTool::right_click_popup_gui(Toolbar& t, Vector2f popupPos) {
    t.paint_popup(popupPos);
}

void ScreenshotTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(button.button == InputManager::MouseButton::LEFT) {
        switch(controls.selectionMode) {
            case ScreenshotControls::SelectionMode::NO_SELECTION: {
                if(button.down && !drawP.world.main.g.gui.cursor_obstructed()) {
                    controls.rectX1 = controls.rectX2 = button.pos.x();
                    controls.rectY1 = controls.rectY2 = button.pos.y();
                    controls.coords = drawP.world.drawData.cam.c;
                    controls.dragType = 0;
                    controls.selectionMode = ScreenshotControls::SelectionMode::DRAGGING_BORDER;
                    drawP.world.main.g.gui.set_to_layout();
                }
                break;
            }
            case ScreenshotControls::SelectionMode::DRAGGING_BORDER: {
                if(!button.down && dragging_border_update(button.pos)) {
                    controls.selectionMode = ScreenshotControls::SelectionMode::SELECTION_EXISTS;
                    commit_rect();
                    drawP.world.main.g.gui.set_to_layout();
                }
                break;
            }
            case ScreenshotControls::SelectionMode::SELECTION_EXISTS: {
                if(button.down && selection_exists_update() && !drawP.world.main.g.gui.cursor_obstructed()) {
                    for(int i = 0; i < 8; i++)
                        if(SCollision::collide(controls.circles[i], drawP.world.main.input.mouse.pos))
                            controls.dragType = i;

                    if(controls.dragType == -1) {
                        Vector2f mousePosScreenshotCoords = controls.coords.from_cam_space_to_this(drawP.world, button.pos);
                        if(SCollision::collide(mousePosScreenshotCoords, SCollision::AABB<float>({controls.rectX1, controls.rectY1}, {controls.rectX2, controls.rectY2})))
                            controls.dragType = -2;
                    }

                    if(controls.dragType == -1)
                        controls.selectionMode = ScreenshotControls::SelectionMode::NO_SELECTION;
                    else if(controls.dragType == -2) {
                        controls.translateBeginPos = drawP.world.drawData.cam.c.from_space(button.pos);
                        controls.translateBeginCoords = controls.coords;
                        controls.selectionMode = ScreenshotControls::SelectionMode::DRAGGING_AREA;
                    }
                    else
                        controls.selectionMode = ScreenshotControls::SelectionMode::DRAGGING_BORDER;
                    drawP.world.main.g.gui.set_to_layout();
                }
                break;
            }
            case ScreenshotControls::SelectionMode::DRAGGING_AREA: {
                if(!button.down && dragging_border_update(button.pos)) {
                    controls.selectionMode = ScreenshotControls::SelectionMode::SELECTION_EXISTS;
                    commit_rect();
                    drawP.world.main.g.gui.set_to_layout();
                }
                break;
            }
        }
    }
}

void ScreenshotTool::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    switch(controls.selectionMode) {
        case ScreenshotControls::SelectionMode::NO_SELECTION: {
            break;
        }
        case ScreenshotControls::SelectionMode::DRAGGING_BORDER: {
            dragging_border_update(motion.pos);
            drawP.world.main.g.gui.set_to_layout();
            break;
        }
        case ScreenshotControls::SelectionMode::SELECTION_EXISTS: {
            break;
        }
        case ScreenshotControls::SelectionMode::DRAGGING_AREA: {
            dragging_area_update(motion.pos);
            break;
        }
    }
}

bool ScreenshotTool::dragging_border_update(const Vector2f& camCursorPos) {
    auto& screenshotConfig = drawP.world.main.toolConfig.screenshot;
    Vector2f curPos = controls.coords.from_cam_space_to_this(drawP.world, camCursorPos);
    float oldX1 = controls.rectX1;
    float oldX2 = controls.rectX2;
    float oldY1 = controls.rectY1;
    float oldY2 = controls.rectY2;
    switch(controls.dragType) {
        case 0:
            controls.rectX2 = curPos.x();
            controls.rectY2 = curPos.y();
            break;
        case 1:
            controls.rectY2 = curPos.y();
            break;
        case 2:
            controls.rectX1 = curPos.x();
            controls.rectY2 = curPos.y();
            break;
        case 3:
            controls.rectX1 = curPos.x();
            break;
        case 4:
            controls.rectX1 = curPos.x();
            controls.rectY1 = curPos.y();
            break;
        case 5:
            controls.rectY1 = curPos.y();
            break;
        case 6:
            controls.rectX2 = curPos.x();
            controls.rectY1 = curPos.y();
            break;
        case 7:
            controls.rectX2 = curPos.x();
            break;
    }
    if(std::fabs(controls.rectX1 - controls.rectX2) > 1e10 || std::fabs(controls.rectY1 - controls.rectY2) > 1e10) { // Limit before screenshot tool starts breaking
        controls.rectX1 = oldX1;
        controls.rectX2 = oldX2;
        controls.rectY1 = oldY1;
        controls.rectY2 = oldY2;
    }
    if((drawP.world.drawData.cam.c.inverseScale << 8) < controls.coords.inverseScale || (drawP.world.drawData.cam.c.inverseScale >> 15) > controls.coords.inverseScale) {
        controls.selectionMode = ScreenshotControls::SelectionMode::NO_SELECTION;
        return false;
    }

    float tempX1 = std::min(controls.rectX1, controls.rectX2);
    float tempX2 = std::max(controls.rectX1, controls.rectX2);
    float tempY1 = std::min(controls.rectY1, controls.rectY2);
    float tempY2 = std::max(controls.rectY1, controls.rectY2);
    if(screenshotConfig.setDimensionIsX) {
        controls.imageSize.x() = screenshotConfig.setDimensionSize;
        controls.imageSize.y() = controls.imageSize.x() * (tempY2 - tempY1) / (tempX2 - tempX1);
    }
    else {
        controls.imageSize.y() = screenshotConfig.setDimensionSize;
        controls.imageSize.x() = controls.imageSize.y() * (tempX2 - tempX1) / (tempY2 - tempY1);
    }

    return true;
}

bool ScreenshotTool::selection_exists_update() {
    commit_rect();
    controls.dragType = -1;

    if((drawP.world.drawData.cam.c.inverseScale << 8) < controls.coords.inverseScale || (drawP.world.drawData.cam.c.inverseScale >> 15) > controls.coords.inverseScale) {
        controls.selectionMode = ScreenshotControls::SelectionMode::NO_SELECTION;
        return false;
    }
    return true;
}

bool ScreenshotTool::dragging_area_update(const Vector2f& camCursorPos) {
    controls.coords = controls.translateBeginCoords;
    controls.coords.translate(drawP.world.drawData.cam.c.from_space(camCursorPos) - controls.translateBeginPos);
    if((drawP.world.drawData.cam.c.inverseScale << 8) < controls.coords.inverseScale || (drawP.world.drawData.cam.c.inverseScale >> 15) > controls.coords.inverseScale) {
        controls.selectionMode = ScreenshotControls::SelectionMode::NO_SELECTION;
        return false;
    }
    return true;
}

void ScreenshotTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
}

void ScreenshotTool::take_screenshot(const std::filesystem::path& filePath, WorldScreenshotInfo::ScreenshotType screenshotType) {
    if(controls.imageSize.x() <= 0 || controls.imageSize.y() <= 0) {
        std::cout << "[ScreenshotTool::take_screenshot] Image size is 0 or negative" << std::endl;
        return;
    }

    world_take_screenshot(drawP.world.main.world, {
        .filePath = filePath,
        .type = screenshotType,
        .imageSizePixels = controls.imageSize,
        .cameraCoords = controls.coords,
        .imageBounds = {{controls.rectX1, controls.rectY1}, {controls.rectX2, controls.rectY2}},
        .transparentBackground = controls.transparentBackground,
        .displayGrid = controls.displayGrid
    });
}

void ScreenshotTool::switch_tool(DrawingProgramToolType newTool) {
    controls.selectionMode = ScreenshotControls::SelectionMode::NO_SELECTION;
}

void ScreenshotTool::tool_update() {
    if(controls.setToTakeScreenshot) {
        controls.setToTakeScreenshot = false;
        take_screenshot(controls.screenshotSavePath, controls.screenshotSaveType);
    }
    if(controls.selectionMode == ScreenshotControls::SelectionMode::SELECTION_EXISTS)
        selection_exists_update();
}

void ScreenshotTool::commit_rect() {
    float x1 = controls.rectX1;
    float x2 = controls.rectX2;
    float y1 = controls.rectY1;
    float y2 = controls.rectY2;
    controls.rectX1 = std::min(x1, x2);
    controls.rectX2 = std::max(x1, x2);
    controls.rectY1 = std::min(y1, y2);
    controls.rectY2 = std::max(y1, y2);

    controls.circles[0] = {drawP.world.drawData.cam.c.to_space(controls.coords.from_space({controls.rectX2                          , controls.rectY2})), drawP.drag_point_radius()};
    controls.circles[1] = {drawP.world.drawData.cam.c.to_space(controls.coords.from_space({(controls.rectX1 + controls.rectX2) * 0.5, controls.rectY2})), drawP.drag_point_radius()};
    controls.circles[2] = {drawP.world.drawData.cam.c.to_space(controls.coords.from_space({controls.rectX1                          , controls.rectY2})), drawP.drag_point_radius()};
    controls.circles[3] = {drawP.world.drawData.cam.c.to_space(controls.coords.from_space({controls.rectX1, (controls.rectY1 + controls.rectY2) * 0.5})), drawP.drag_point_radius()};
    controls.circles[4] = {drawP.world.drawData.cam.c.to_space(controls.coords.from_space({controls.rectX1                          , controls.rectY1})), drawP.drag_point_radius()};
    controls.circles[5] = {drawP.world.drawData.cam.c.to_space(controls.coords.from_space({(controls.rectX1 + controls.rectX2) * 0.5, controls.rectY1})), drawP.drag_point_radius()};
    controls.circles[6] = {drawP.world.drawData.cam.c.to_space(controls.coords.from_space({controls.rectX2                          , controls.rectY1})), drawP.drag_point_radius()};
    controls.circles[7] = {drawP.world.drawData.cam.c.to_space(controls.coords.from_space({controls.rectX2, (controls.rectY1 + controls.rectY2) * 0.5})), drawP.drag_point_radius()};
}

bool ScreenshotTool::prevent_undo_or_redo() {
    return false;
}

void ScreenshotTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(controls.selectionMode == ScreenshotControls::SelectionMode::NO_SELECTION) {
        SkPaint paint1;
        paint1.setColor4f({0.0f, 0.0f, 0.0f, 0.3f});
        canvas->drawPaint(paint1);
    }
    else {
        canvas->save();
        controls.coords.transform_sk_canvas(canvas, drawData);
        float x1 = std::min(controls.rectX1, controls.rectX2);
        float x2 = std::max(controls.rectX1, controls.rectX2);
        float y1 = std::min(controls.rectY1, controls.rectY2);
        float y2 = std::max(controls.rectY1, controls.rectY2);
        SkPoint p1{x1, y1};
        SkPoint p2{x2, y2};
        SkPathBuilder path1B;
        path1B.addRect(SkRect::MakeLTRB(p1.x(), p1.y(), p2.x(), p2.y()));
        path1B.setFillType(SkPathFillType::kInverseWinding);
        SkPath path1 = path1B.detach();
        SkPaint paint1;
        paint1.setColor4f({0.0f, 0.0f, 0.0f, 0.3f});
        canvas->drawPath(path1, paint1);

        path1.setFillType(SkPathFillType::kWinding);
        SkPaint paint2;
        paint2.setColor4f({1.0f, 1.0f, 1.0f, 0.9f});
        paint2.setStyle(SkPaint::kStroke_Style);
        canvas->drawPath(path1, paint2);

        canvas->restore();

        if(controls.selectionMode == ScreenshotControls::SelectionMode::SELECTION_EXISTS) {
            paint2.setStyle(SkPaint::kFill_Style);
            for(auto& circ : controls.circles)
                drawP.draw_drag_circle(canvas, circ.pos, {0.1f, 0.9f, 0.9f, 1.0f}, drawData, 1.0f);
        }
    }
}
