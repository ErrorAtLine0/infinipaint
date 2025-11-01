#include "ScreenshotTool.hpp"
#include "DrawingProgram.hpp"
#include "../Server/CommandList.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
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
#include <include/svg/SkSVGCanvas.h>
#include <include/encode/SkPngEncoder.h>
#include <include/encode/SkWebpEncoder.h>
#include <include/encode/SkJpegEncoder.h>
#include <fstream>
#include <include/gpu/GpuTypes.h>
#include "../FileHelpers.hpp"
#include <filesystem>
#ifdef USE_SKIA_BACKEND_GRAPHITE
    #include <include/gpu/graphite/Surface.h>
#elif USE_SKIA_BACKEND_GANESH
    #include <include/gpu/ganesh/GrDirectContext.h>
    #include <include/gpu/ganesh/SkSurfaceGanesh.h>
#endif
#include "../GridManager.hpp"
#include <Helpers/Logger.hpp>

#include "../MainProgram.hpp"

#ifdef __EMSCRIPTEN__
    #include <EmscriptenHelpers/emscripten_browser_file.h>
#endif

#define SECTION_SIZE 4000
#define AA_LEVEL 4

ScreenshotTool::ScreenshotTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{}

DrawingProgramToolType ScreenshotTool::get_type() {
    return DrawingProgramToolType::SCREENSHOT;
}

void ScreenshotTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("screenshot tool");
    t.gui.text_label_centered("Screenshot");
    auto oldImgSize = controls.imageSize;
    if(controls.selectionMode == 0)
        t.gui.text_label("Select an area on the canvas...");
    if(controls.selectionMode != 0 && controls.selectedType != SCREENSHOT_SVG)
        t.gui.input_scalar_fields("Image Size", "Image Size", &controls.imageSize, 2, 0, 999999999);
    if(controls.selectionMode == 2) {
        t.gui.left_to_right_line_layout([&]() {
            t.gui.text_label("Image Type");
            t.gui.dropdown_select("image type select", &controls.selectedType, controls.typeSelections);
        });
        if(controls.selectedType != SCREENSHOT_SVG)
            t.gui.checkbox_field("Display Grid", "Display Grid", &controls.displayGrid);
        if(controls.selectedType != 0)
            t.gui.checkbox_field("Transparent Background", "Transparent Background", &controls.transparentBackground);
        if(controls.imageSize.x() != oldImgSize.x()) {
            controls.setDimensionSize = controls.imageSize.x();
            controls.setDimensionIsX = true;
            controls.imageSize.y() = controls.imageSize.x() * (controls.rectY2 - controls.rectY1) / (controls.rectX2 - controls.rectX1);
        }
        else if(controls.imageSize.y() != oldImgSize.y()) {
            controls.setDimensionSize = controls.imageSize.y();
            controls.setDimensionIsX = false;
            controls.imageSize.x() = controls.imageSize.y() * (controls.rectX2 - controls.rectX1) / (controls.rectY2 - controls.rectY1);
        }
        if(t.gui.text_button_wide("Take Screenshot", "Take Screenshot")) {
            controls.selectionMode = 0;
            #ifdef __EMSCRIPTEN__
                take_screenshot("a" + controls.typeSelections[controls.selectedType]);
            #else
                // We can't actually use the extension from the callback, so we have to set the extension of choice beforehand
                Toolbar::ExtensionFilter setExtensionFilter;
                switch(controls.selectedType) {
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
                        screenshotTool->controls.screenshotSavePath = force_extension_on_path(p, setExtensionFilter.extensions);
                        screenshotTool->controls.setToTakeScreenshot = true;
                    }
                }, "screenshot", true);
            #endif
        }
    }
    t.gui.pop_id();
}

void ScreenshotTool::take_screenshot(const std::filesystem::path& filePath) {
    if(controls.imageSize.x() <= 0 || controls.imageSize.y() <= 0) {
        std::cout << "[ScreenshotTool::take_screenshot] Image size is 0 or negative" << std::endl;
        return;
    }

    std::string ext = filePath.extension().string();
    size_t extType = SCREENSHOT_JPG;
    if(ext == ".jpg" || ext == ".jpeg")
        extType = SCREENSHOT_JPG;
    else if(ext == ".png")
        extType = SCREENSHOT_PNG;
    else if(ext == ".webp")
        extType = SCREENSHOT_WEBP;
    else if(ext == ".svg")
        extType = SCREENSHOT_SVG;

    if(extType != SCREENSHOT_SVG) {
        size_t imageByteSize = (size_t)controls.imageSize.x() * (size_t)controls.imageSize.y() * 4;
        size_t imageRowSize = 4 * controls.imageSize.x();
    
        SkImageInfo finalImgInfo = SkImageInfo::Make(controls.imageSize.x(), controls.imageSize.y(), kRGBA_8888_SkColorType, kUnpremul_SkAlphaType);
        std::vector<uint8_t> finalImgRawData(imageByteSize);
        SkPixmap finalImgData(finalImgInfo, finalImgRawData.data(), imageRowSize);
        
        #ifdef USE_BACKEND_OPENGLES_3_0
            SkImageInfo imgInfo = SkImageInfo::Make(drawP.world.main.window.size.x(), drawP.world.main.window.size.y(), kRGBA_8888_SkColorType, kPremul_SkAlphaType);
        #else
            SkImageInfo imgInfo = SkImageInfo::MakeN32Premul(drawP.world.main.window.size.x(), drawP.world.main.window.size.y());
        #endif
        #ifdef USE_SKIA_BACKEND_GRAPHITE
            sk_sp<SkSurface> surface = SkSurfaces::RenderTarget(drawP.world.main.window.recorder(), imgInfo, skgpu::Mipmapped::kNo, &drawP.world.main.window.defaultMSAASurfaceProps);
        #elif USE_SKIA_BACKEND_GANESH
            sk_sp<SkSurface> surface = SkSurfaces::RenderTarget(drawP.world.main.window.ctx.get(), skgpu::Budgeted::kNo, imgInfo, drawP.world.main.window.defaultMSAASampleCount, &drawP.world.main.window.defaultMSAASurfaceProps);
        #endif
        if(!surface) {
            Logger::get().log("INFO", "[SkSurfaces::WrapBackendRenderTarget] Screenshot Tool could not make surface");
            return;
        }
        SkCanvas* screenshotCanvas = surface->getCanvas();
        if(!screenshotCanvas) {
            Logger::get().log("INFO", "Screenshot Tool could not make canvas");
            return;
        }
    
        for(int i = 0; i < controls.imageSize.x(); i += drawP.world.main.window.size.x())
            for(int j = 0; j < controls.imageSize.y(); j += drawP.world.main.window.size.y())
                take_screenshot_area_hw(surface, screenshotCanvas, finalImgRawData.data(), controls.imageSize, Vector2i{i, j}, Vector2i{std::min(drawP.world.main.window.size.x(), controls.imageSize.x() - i), std::min(drawP.world.main.window.size.y(), controls.imageSize.y() - j)}, drawP.world.main.window.size, extType != 0 && controls.transparentBackground);
    
        bool success = false;
        SkDynamicMemoryWStream out;
    
        switch(extType) {
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
            std::ofstream fi(filePath, std::ios::out | std::ios::binary);
            auto skData = out.detachAsData();
            fi.write((const char*)skData->bytes(), skData->size());
            fi.close();
        }
        catch(const std::exception& e) {
            Logger::get().log("WORLDFATAL", std::string("[ScreenshotTool::take_screenshot] Save screenshot error: ") + e.what());
        }
    #else
        if(success) {
            std::string mimeTypeArray[] = {"image/jpeg", "image/png", "image/webp"};
            auto skData = out.detachAsData();
            emscripten_browser_file::download(
                "screenshot" + controls.typeSelections[extType],
                mimeTypeArray[extType],
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
            std::ofstream fi(filePath, std::ios::out | std::ios::binary);
            auto skData = out.detachAsData();
            fi.write((const char*)skData->bytes(), skData->size());
            fi.close();
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

    DrawData d = drawP.world.drawData;
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

    drawP.world.drawData.cam.set_based_on_properties(drawP.world, topLeft, newInverseScale, controls.coords.rotation);
    drawP.world.drawData.cam.set_viewing_area(canvasSize);
    drawP.world.drawData.refresh_draw_optimizing_values();

    bool oldDrawGrids = drawP.world.drawData.drawGrids;
    drawP.world.main.takingScreenshot = true;
    drawP.world.main.transparentBackground = transparentBackground;
    drawP.world.drawData.drawGrids = false;
    drawP.world.drawData.dontUseDrawProgCache = true;

    drawP.world.main.draw(canvas);
    drawP.world.main.takingScreenshot = false;
    drawP.world.main.transparentBackground = false;
    drawP.world.drawData.drawGrids = oldDrawGrids;
    drawP.world.drawData = d;
}

void ScreenshotTool::take_screenshot_area_hw(const sk_sp<SkSurface>& surface, SkCanvas* canvas, void* fullImgRawData, const Vector2i& fullImageSize, const Vector2i& sectionImagePos, const Vector2i& sectionImageSize, const Vector2i& canvasSize, bool transparentBackground) {
    //std::cout << "[ScreenshotTool::take_screenshot_area_hw] Screenshot Pos: " << "[" << sectionImagePos.x() << " " << sectionImagePos.y() << "]" << std::endl;

    float secRectX1 = controls.rectX1 + (controls.rectX2 - controls.rectX1) * (sectionImagePos.x() / (double)fullImageSize.x());
    float secRectX2 = controls.rectX1 + (controls.rectX2 - controls.rectX1) * ((sectionImagePos.x() + canvasSize.x()) / (double)fullImageSize.x());
    float secRectY1 = controls.rectY1 + (controls.rectY2 - controls.rectY1) * (sectionImagePos.y() / (double)fullImageSize.y());
    float secRectY2 = controls.rectY1 + (controls.rectY2 - controls.rectY1) * ((sectionImagePos.y() + canvasSize.y()) / (double)fullImageSize.y());

    DrawData d = drawP.world.drawData;
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

    drawP.world.drawData.cam.set_based_on_properties(drawP.world, topLeft, newInverseScale, controls.coords.rotation);
    drawP.world.drawData.cam.set_viewing_area(sectionImageSize.cast<float>());
    drawP.world.drawData.refresh_draw_optimizing_values();

    bool oldDrawGrids = drawP.world.drawData.drawGrids;
    drawP.world.main.takingScreenshot = true;
    drawP.world.main.transparentBackground = transparentBackground;
    drawP.world.drawData.drawGrids = controls.displayGrid;
    drawP.world.drawData.dontUseDrawProgCache = true;

    drawP.world.main.draw(canvas);
    drawP.world.main.takingScreenshot = false;
    drawP.world.main.transparentBackground = false;
    drawP.world.drawData.drawGrids = oldDrawGrids;
    drawP.world.drawData = d;

    SkImageInfo aaImgInfo = SkImageInfo::Make(sectionImageSize.x(), sectionImageSize.y(), kRGBA_8888_SkColorType, kPremul_SkAlphaType);
    void* fullImgRawDataStartPt = (uint8_t*)fullImgRawData + 4 * (size_t)sectionImagePos.x() + 4 * (size_t)fullImageSize.x() * (size_t)sectionImagePos.y();
    SkPixmap aaImgData(aaImgInfo, fullImgRawDataStartPt, fullImageSize.x() * 4);
    if(!surface->readPixels(aaImgData, 0, 0))
        throw std::runtime_error("[ScreenshotTool::take_screenshot_area_hw] Error copy pixmap");
}

void ScreenshotTool::switch_tool(DrawingProgramToolType newTool) {
    controls.selectionMode = 0;
}

void ScreenshotTool::tool_update() {
    if(controls.setToTakeScreenshot) {
        controls.setToTakeScreenshot = false;
        take_screenshot(controls.screenshotSavePath);
    }
    switch(controls.selectionMode) {
        case 0: {
            if(drawP.controls.leftClick) {
                controls.rectX1 = controls.rectX2 = drawP.world.main.input.mouse.pos.x();
                controls.rectY1 = controls.rectY2 = drawP.world.main.input.mouse.pos.y();
                controls.coords = drawP.world.drawData.cam.c;
                controls.dragType = 0;
                controls.selectionMode = 1;
            }
            break;
        }
        case 1: {
            Vector2f curPos = controls.coords.get_mouse_pos(drawP.world);
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
                controls.selectionMode = 0;
                break;
            }

            float tempX1 = std::min(controls.rectX1, controls.rectX2);
            float tempX2 = std::max(controls.rectX1, controls.rectX2);
            float tempY1 = std::min(controls.rectY1, controls.rectY2);
            float tempY2 = std::max(controls.rectY1, controls.rectY2);
            if(controls.setDimensionIsX) {
                controls.imageSize.x() = controls.setDimensionSize;
                controls.imageSize.y() = controls.imageSize.x() * (tempY2 - tempY1) / (tempX2 - tempX1);
            }
            else {
                controls.imageSize.y() = controls.setDimensionSize;
                controls.imageSize.x() = controls.imageSize.y() * (tempX2 - tempX1) / (tempY2 - tempY1);
            }

            if(!drawP.controls.leftClickHeld) {
                controls.selectionMode = 2;
                commit_rect();
            }
            break;
        }
        case 2: {
            commit_rect();
            controls.dragType = -1;

            if((drawP.world.drawData.cam.c.inverseScale << 8) < controls.coords.inverseScale || (drawP.world.drawData.cam.c.inverseScale >> 15) > controls.coords.inverseScale) {
                controls.selectionMode = 0;
                break;
            }

            if(drawP.controls.cursorHoveringOverCanvas && drawP.controls.leftClick) {
                for(int i = 0; i < 8; i++)
                    if(SCollision::collide(controls.circles[i], drawP.world.main.input.mouse.pos))
                        controls.dragType = i;

                if(controls.dragType == -1) {
                    Vector2f mousePosScreenshotCoords = controls.coords.get_mouse_pos(drawP.world);
                    if(SCollision::collide(mousePosScreenshotCoords, SCollision::AABB<float>({controls.rectX1, controls.rectY1}, {controls.rectX2, controls.rectY2})))
                        controls.dragType = -2;
                }

                if(controls.dragType == -1)
                    controls.selectionMode = 0;
                else if(controls.dragType == -2) {
                    controls.translateBeginPos = drawP.world.get_mouse_world_pos();
                    controls.translateBeginCoords = controls.coords;
                    controls.selectionMode = 3;
                }
                else
                    controls.selectionMode = 1;
            }
            break;
        }
        case 3: {
            controls.coords = controls.translateBeginCoords;
            controls.coords.translate(drawP.world.get_mouse_world_pos() - controls.translateBeginPos);
            if((drawP.world.drawData.cam.c.inverseScale << 8) < controls.coords.inverseScale || (drawP.world.drawData.cam.c.inverseScale >> 15) > controls.coords.inverseScale) {
                controls.selectionMode = 0;
                break;
            }
            if(!drawP.controls.leftClickHeld) {
                controls.selectionMode = 2;
                commit_rect();
            }
            break;
        }
    }
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

    controls.circles[0] = {drawP.world.drawData.cam.c.to_space(controls.coords.from_space({controls.rectX2                          , controls.rectY2})), DRAG_POINT_RADIUS};
    controls.circles[1] = {drawP.world.drawData.cam.c.to_space(controls.coords.from_space({(controls.rectX1 + controls.rectX2) * 0.5, controls.rectY2})), DRAG_POINT_RADIUS};
    controls.circles[2] = {drawP.world.drawData.cam.c.to_space(controls.coords.from_space({controls.rectX1                          , controls.rectY2})), DRAG_POINT_RADIUS};
    controls.circles[3] = {drawP.world.drawData.cam.c.to_space(controls.coords.from_space({controls.rectX1, (controls.rectY1 + controls.rectY2) * 0.5})), DRAG_POINT_RADIUS};
    controls.circles[4] = {drawP.world.drawData.cam.c.to_space(controls.coords.from_space({controls.rectX1                          , controls.rectY1})), DRAG_POINT_RADIUS};
    controls.circles[5] = {drawP.world.drawData.cam.c.to_space(controls.coords.from_space({(controls.rectX1 + controls.rectX2) * 0.5, controls.rectY1})), DRAG_POINT_RADIUS};
    controls.circles[6] = {drawP.world.drawData.cam.c.to_space(controls.coords.from_space({controls.rectX2                          , controls.rectY1})), DRAG_POINT_RADIUS};
    controls.circles[7] = {drawP.world.drawData.cam.c.to_space(controls.coords.from_space({controls.rectX2, (controls.rectY1 + controls.rectY2) * 0.5})), DRAG_POINT_RADIUS};
}

bool ScreenshotTool::prevent_undo_or_redo() {
    return false;
}

void ScreenshotTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(!drawData.main->takingScreenshot) {
        if(controls.selectionMode == 0) {
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
            SkPath path1;
            path1.addRect(p1.x(), p1.y(), p2.x(), p2.y());
            path1.setFillType(SkPathFillType::kInverseWinding);
            SkPaint paint1;
            paint1.setColor4f({0.0f, 0.0f, 0.0f, 0.3f});
            canvas->drawPath(path1, paint1);

            path1.setFillType(SkPathFillType::kWinding);
            SkPaint paint2;
            paint2.setColor4f({1.0f, 1.0f, 1.0f, 0.9f});
            paint2.setStyle(SkPaint::kStroke_Style);
            canvas->drawPath(path1, paint2);

            canvas->restore();

            if(controls.selectionMode == 2) {
                paint2.setStyle(SkPaint::kFill_Style);
                for(auto& circ : controls.circles)
                    drawP.draw_drag_circle(canvas, circ.pos, {0.1f, 0.9f, 0.9f, 1.0f}, drawData, 1.0f);
            }
        }
    }
}
