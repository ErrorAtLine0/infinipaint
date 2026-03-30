#include "ScreenshotTool.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
#include "DrawingProgramToolBase.hpp"
#include "Helpers/SCollision.hpp"
#include <include/core/SkAlphaType.h>
#include <include/core/SkColorType.h>
#include <include/core/SkPaint.h>
#include <include/core/SkPathTypes.h>
#include <include/core/SkStream.h>
#include <include/core/SkSurface.h>
#include <include/core/SkImage.h>
#include <include/core/SkData.h>
#include <include/core/SkPathBuilder.h>
#include <include/svg/SkSVGCanvas.h>
#include <include/encode/SkPngEncoder.h>
#include <include/encode/SkWebpEncoder.h>
#include <include/encode/SkJpegEncoder.h>
#include <fstream>
#include <include/gpu/GpuTypes.h>
#include "../../FileHelpers.hpp"
#include <filesystem>
#ifdef USE_SKIA_BACKEND_GRAPHITE
    #include <include/gpu/graphite/Surface.h>
#elif USE_SKIA_BACKEND_GANESH
    #include <include/gpu/ganesh/GrDirectContext.h>
    #include <include/gpu/ganesh/SkSurfaceGanesh.h>
#endif
#include "../../GridManager.hpp"
#include <Helpers/Logger.hpp>

#include "../../MainProgram.hpp"

#ifdef __EMSCRIPTEN__
    #include <EmscriptenHelpers/emscripten_browser_file.h>
#endif

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

void ScreenshotTool::gui_toolbox() {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    Toolbar& t = drawP.world.main.toolbar;
    auto& screenshotConfig = drawP.world.main.toolConfig.screenshot;
    t.gui.new_id("screenshot tool", [&] {
        text_label_centered(t.gui, "Screenshot");
        auto oldImgSize = controls.imageSize;
        if(controls.selectionMode == ScreenshotControls::SelectionMode::NO_SELECTION)
            text_label(t.gui, "Select an area on the canvas...");
        if(controls.selectionMode != ScreenshotControls::SelectionMode::NO_SELECTION && screenshotConfig.selectedType != SCREENSHOT_SVG)
            input_scalars_field(t.gui, "Image Size", "Image Size", &controls.imageSize, 2, 0, 999999999);
        if(controls.selectionMode == ScreenshotControls::SelectionMode::SELECTION_EXISTS) {
            left_to_right_line_layout(t.gui, [&]() {
                text_label(t.gui, "Image Type");
                t.gui.element<DropDown<size_t>>("image type select", (size_t*)(&screenshotConfig.selectedType), controls.typeSelections);
            });
            if(screenshotConfig.selectedType != SCREENSHOT_SVG)
                checkbox_boolean_field(t.gui, "Display Grid", "Display Grid", &controls.displayGrid);
            else
                text_label(t.gui, "Note: Screenshot will ignore blend\nmodes and layer alpha");
            if(screenshotConfig.selectedType != 0)
                checkbox_boolean_field(t.gui, "Transparent Background", "Transparent Background", &controls.transparentBackground);
            if(controls.imageSize.x() != oldImgSize.x()) {
                screenshotConfig.setDimensionSize = controls.imageSize.x();
                screenshotConfig.setDimensionIsX = true;
                controls.imageSize.y() = controls.imageSize.x() * (controls.rectY2 - controls.rectY1) / (controls.rectX2 - controls.rectX1);
            }
            else if(controls.imageSize.y() != oldImgSize.y()) {
                screenshotConfig.setDimensionSize = controls.imageSize.y();
                screenshotConfig.setDimensionIsX = false;
                controls.imageSize.x() = controls.imageSize.y() * (controls.rectX2 - controls.rectX1) / (controls.rectY2 - controls.rectY1);
            }
            text_button(t.gui, "Take Screenshot", "Take Screenshot", {
                .wide = true,
                .onClick = [&] {
                    controls.selectionMode = ScreenshotControls::SelectionMode::NO_SELECTION;
                    #ifdef __EMSCRIPTEN__
                        take_screenshot("a" + controls.typeSelections[screenshotConfig.selectedType], screenshotConfig.selectedType);
                    #else
                        // We can't actually use the extension from the callback, so we have to set the extension of choice beforehand
                        Toolbar::ExtensionFilter setExtensionFilter;
                        switch(screenshotConfig.selectedType) {
                            case SCREENSHOT_JPG:
                                setExtensionFilter = {"JPEG", "jpg;jpeg"};
                                break;
                            case SCREENSHOT_PNG:
                                setExtensionFilter = {"PNG", "png"};
                                break;
                            case SCREENSHOT_WEBP:
                                setExtensionFilter = {"WEBP", "webp"};
                                break;
                            case SCREENSHOT_SVG:
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

void ScreenshotTool::right_click_popup_gui(Vector2f popupPos) {
    Toolbar& t = drawP.world.main.toolbar;
    t.paint_popup(popupPos);
}

void ScreenshotTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(button.button == InputManager::MouseButton::LEFT) {
        switch(controls.selectionMode) {
            case ScreenshotControls::SelectionMode::NO_SELECTION: {
                if(button.down) {
                    controls.rectX1 = controls.rectX2 = button.pos.x();
                    controls.rectY1 = controls.rectY2 = button.pos.y();
                    controls.coords = drawP.world.drawData.cam.c;
                    controls.dragType = 0;
                    controls.selectionMode = ScreenshotControls::SelectionMode::DRAGGING_BORDER;
                }
                break;
            }
            case ScreenshotControls::SelectionMode::DRAGGING_BORDER: {
                if(!button.down && dragging_border_update(button.pos)) {
                    controls.selectionMode = ScreenshotControls::SelectionMode::SELECTION_EXISTS;
                    commit_rect();
                }
                break;
            }
            case ScreenshotControls::SelectionMode::SELECTION_EXISTS: {
                if(button.down && selection_exists_update()) {
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
                }
                break;
            }
            case ScreenshotControls::SelectionMode::DRAGGING_AREA: {
                if(!button.down && dragging_border_update(button.pos)) {
                    controls.selectionMode = ScreenshotControls::SelectionMode::SELECTION_EXISTS;
                    commit_rect();
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

void ScreenshotTool::take_screenshot(const std::filesystem::path& filePath, ScreenshotType screenshotType) {
    if(controls.imageSize.x() <= 0 || controls.imageSize.y() <= 0) {
        std::cout << "[ScreenshotTool::take_screenshot] Image size is 0 or negative" << std::endl;
        return;
    }

    if(screenshotType != SCREENSHOT_SVG) {
        size_t imageByteSize = (size_t)controls.imageSize.x() * (size_t)controls.imageSize.y() * 4;
        size_t imageRowSize = 4 * controls.imageSize.x();
    
        SkImageInfo finalImgInfo = SkImageInfo::Make(controls.imageSize.x(), controls.imageSize.y(), kRGBA_8888_SkColorType, kUnpremul_SkAlphaType);
        std::vector<uint8_t> finalImgRawData(imageByteSize);
        SkPixmap finalImgData(finalImgInfo, finalImgRawData.data(), imageRowSize);
        
        sk_sp<SkSurface> surface = drawP.world.main.create_native_surface(drawP.world.main.window.size, true);
        SkCanvas* screenshotCanvas = surface->getCanvas();
        if(!screenshotCanvas) {
            Logger::get().log("INFO", "Screenshot Tool could not make canvas");
            return;
        }
    
        for(int i = 0; i < controls.imageSize.x(); i += drawP.world.main.window.size.x())
            for(int j = 0; j < controls.imageSize.y(); j += drawP.world.main.window.size.y())
                take_screenshot_area_hw(surface, screenshotCanvas, finalImgRawData.data(), controls.imageSize, Vector2i{i, j}, Vector2i{std::min(drawP.world.main.window.size.x(), controls.imageSize.x() - i), std::min(drawP.world.main.window.size.y(), controls.imageSize.y() - j)}, drawP.world.main.window.size, screenshotType != SCREENSHOT_JPG && controls.transparentBackground);
    
        bool success = false;
        SkDynamicMemoryWStream out;
    
        switch(screenshotType) {
            case SCREENSHOT_JPG:
                success = SkJpegEncoder::Encode(&out, finalImgData, {});
                break;
            case SCREENSHOT_PNG:
                success = SkPngEncoder::Encode(&out, finalImgData, {});
                break;
            case SCREENSHOT_WEBP:
                success = SkWebpEncoder::Encode(&out, finalImgData, {});
                break;
            default:
                break;
        }
        if(!success) {
            Logger::get().log("WORLDFATAL", "[ScreenshotTool::take_screenshot] Could not encode and write screenshot");
            return;
        }
        out.flush();

    #ifndef __EMSCRIPTEN__
        try {
            auto skData = out.detachAsData();
            if(!SDL_SaveFile(filePath.c_str(), skData->bytes(), skData->size()))
                throw std::runtime_error("SDL_SaveFile failed with error: " + std::string(SDL_GetError()));
        }
        catch(const std::exception& e) {
            Logger::get().log("WORLDFATAL", std::string("[ScreenshotTool::take_screenshot] Save screenshot error: ") + e.what());
        }
    #else
        if(success) {
            std::string mimeTypeArray[] = {"image/jpeg", "image/png", "image/webp"};
            auto skData = out.detachAsData();
            emscripten_browser_file::download(
                "screenshot" + controls.typeSelections[screenshotType],
                mimeTypeArray[screenshotType],
                std::string_view((const char*)skData->bytes(), skData->size())
            );
        }
    #endif
    }
    else {
        SkDynamicMemoryWStream out;
        Vector2f canvasSize{controls.rectX2 - controls.rectX1, controls.rectY2 - controls.rectY1};
        SkRect canvasBounds = SkRect::MakeLTRB(0.0f, 0.0f, canvasSize.x(), canvasSize.y());
        std::unique_ptr<SkCanvas> canvas = SkSVGCanvas::Make(canvasBounds, &out, SkSVGCanvas::kConvertTextToPaths_Flag | SkSVGCanvas::kNoPrettyXML_Flag);
        take_screenshot_svg(canvas.get(), controls.transparentBackground);
        canvas = nullptr; // Ensure that SVG is completely written into the stream
        out.flush();

    #ifndef __EMSCRIPTEN__
        try {
            auto skData = out.detachAsData();
            if(!SDL_SaveFile(filePath.c_str(), skData->bytes(), skData->size()))
                throw std::runtime_error("SDL_SaveFile failed with error: " + std::string(SDL_GetError()));
        }
        catch(const std::exception& e) {
            Logger::get().log("WORLDFATAL", std::string("[ScreenshotTool::take_screenshot] Save screenshot error: ") + e.what());
        }
    #else
        auto skData = out.detachAsData();
        emscripten_browser_file::download(
            "screenshot.svg",
            "image/svg+xml",
            std::string_view((const char*)skData->bytes(), skData->size())
        );
    #endif
    }
}

void ScreenshotTool::take_screenshot_svg(SkCanvas* canvas, bool transparentBackground) {
    float secRectX1 = controls.rectX1;
    float secRectX2 = controls.rectX2;
    float secRectY1 = controls.rectY1;
    float secRectY2 = controls.rectY2;

    Vector2f canvasSize{controls.rectX2 - controls.rectX1, controls.rectY2 - controls.rectY1};

    WorldVec topLeft = controls.coords.from_space({secRectX1, secRectY1});
    WorldVec topRight = controls.coords.from_space({secRectX2, secRectY1});
    WorldVec bottomLeft = controls.coords.from_space({secRectX1, secRectY2});
    WorldVec bottomRight = controls.coords.from_space({secRectX2, secRectY2});
    WorldVec camCenter = (topLeft + bottomRight) / WorldScalar(2);

    WorldVec vectorZoom;
    WorldScalar distX = (camCenter - (topLeft + bottomLeft) * WorldScalar(0.5)).norm();
    WorldScalar distY = (camCenter - (topLeft + topRight) * WorldScalar(0.5)).norm();
    vectorZoom.x() = distX / WorldScalar(canvasSize.x() * 0.5);
    vectorZoom.y() = distY / WorldScalar(canvasSize.y() * 0.5);
    WorldScalar newInverseScale = (vectorZoom.x() + vectorZoom.y()) * WorldScalar(0.5);

    DrawData screenshotDrawData = drawP.world.drawData;
    screenshotDrawData.cam.set_based_on_properties(drawP.world, topLeft, newInverseScale, controls.coords.rotation);
    screenshotDrawData.cam.set_viewing_area(canvasSize);
    screenshotDrawData.takingScreenshot = true;
    screenshotDrawData.transparentBackground = transparentBackground;
    screenshotDrawData.drawGrids = false;
    screenshotDrawData.isSVGRender = true;
    screenshotDrawData.refresh_draw_optimizing_values();
    drawP.world.main.draw(canvas, drawP.world.main.world, screenshotDrawData);
}

void ScreenshotTool::take_screenshot_area_hw(const sk_sp<SkSurface>& surface, SkCanvas* canvas, void* fullImgRawData, const Vector2i& fullImageSize, const Vector2i& sectionImagePos, const Vector2i& sectionImageSize, const Vector2i& canvasSize, bool transparentBackground) {
    //std::cout << "[ScreenshotTool::take_screenshot_area_hw] Screenshot Pos: " << "[" << sectionImagePos.x() << " " << sectionImagePos.y() << "]" << std::endl;

    float secRectX1 = controls.rectX1 + (controls.rectX2 - controls.rectX1) * (sectionImagePos.x() / (double)fullImageSize.x());
    float secRectX2 = controls.rectX1 + (controls.rectX2 - controls.rectX1) * ((sectionImagePos.x() + canvasSize.x()) / (double)fullImageSize.x());
    float secRectY1 = controls.rectY1 + (controls.rectY2 - controls.rectY1) * (sectionImagePos.y() / (double)fullImageSize.y());
    float secRectY2 = controls.rectY1 + (controls.rectY2 - controls.rectY1) * ((sectionImagePos.y() + canvasSize.y()) / (double)fullImageSize.y());

    WorldVec topLeft = controls.coords.from_space({secRectX1, secRectY1});
    WorldVec topRight = controls.coords.from_space({secRectX2, secRectY1});
    WorldVec bottomLeft = controls.coords.from_space({secRectX1, secRectY2});
    WorldVec bottomRight = controls.coords.from_space({secRectX2, secRectY2});
    WorldVec camCenter = (topLeft + bottomRight) / WorldScalar(2);

    WorldVec vectorZoom;
    WorldScalar distX = (camCenter - (topLeft + bottomLeft) * WorldScalar(0.5)).norm();
    WorldScalar distY = (camCenter - (topLeft + topRight) * WorldScalar(0.5)).norm();
    vectorZoom.x() = distX / WorldScalar(canvasSize.x() * 0.5);
    vectorZoom.y() = distY / WorldScalar(canvasSize.y() * 0.5);
    WorldScalar newInverseScale = (vectorZoom.x() + vectorZoom.y()) * WorldScalar(0.5);

    DrawData screenshotDrawData = drawP.world.drawData;
    screenshotDrawData.cam.set_based_on_properties(drawP.world, topLeft, newInverseScale, controls.coords.rotation);
    screenshotDrawData.cam.set_viewing_area(sectionImageSize.cast<float>());
    screenshotDrawData.takingScreenshot = true;
    screenshotDrawData.transparentBackground = transparentBackground;
    screenshotDrawData.drawGrids = controls.displayGrid;
    screenshotDrawData.refresh_draw_optimizing_values();
    drawP.world.main.draw(canvas, drawP.world.main.world, screenshotDrawData);

    SkImageInfo aaImgInfo = SkImageInfo::Make(sectionImageSize.x(), sectionImageSize.y(), kRGBA_8888_SkColorType, kPremul_SkAlphaType);
    void* fullImgRawDataStartPt = (uint8_t*)fullImgRawData + 4 * (size_t)sectionImagePos.x() + 4 * (size_t)fullImageSize.x() * (size_t)sectionImagePos.y();
    SkPixmap aaImgData(aaImgInfo, fullImgRawDataStartPt, fullImageSize.x() * 4);
    if(!surface->readPixels(aaImgData, 0, 0))
        throw std::runtime_error("[ScreenshotTool::take_screenshot_area_hw] Error copy pixmap");
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
