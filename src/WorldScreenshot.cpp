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

#include "CoordSpaceHelper.hpp"
#include "World.hpp"
#include "MainProgram.hpp"
#include <Helpers/Logger.hpp>
#include "WorldScreenshot.hpp"

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
#include <include/gpu/GpuTypes.h>

#ifdef __EMSCRIPTEN__
    #include <EmscriptenHelpers/emscripten_browser_file.h>
#endif

#ifdef USE_SKIA_BACKEND_GRAPHITE
    #include <include/gpu/graphite/Surface.h>
#elif USE_SKIA_BACKEND_GANESH
    #include <include/gpu/ganesh/GrDirectContext.h>
    #include <include/gpu/ganesh/SkSurfaceGanesh.h>
#endif

void take_screenshot_svg(const std::shared_ptr<World>& w, SkCanvas* canvas, bool transparentBackground, const CoordSpaceHelper& cameraCoords, const SCollision::AABB<float>& imageBounds);
void take_screenshot_area_hw(const std::shared_ptr<World>& w, const sk_sp<SkSurface>& surface, SkCanvas* canvas, void* fullImgRawData, const Vector2i& fullImageSize, const Vector2i& sectionImagePos, const Vector2i& sectionImageSize, const Vector2i& canvasSize, bool transparentBackground, const CoordSpaceHelper& cameraCoords, const SCollision::AABB<float>& imageBounds, bool displayGrid);

void world_take_screenshot(const std::shared_ptr<World>& w, const WorldScreenshotInfo& info) {
    if(info.imageSizePixels.x() <= 0 || info.imageSizePixels.y() <= 0) {
        std::cout << "[ScreenshotTool::take_screenshot] Image size is 0 or negative" << std::endl;
        return;
    }

    if(info.type != WorldScreenshotInfo::ScreenshotType::SVG) {
        size_t imageByteSize = (size_t)info.imageSizePixels.x() * (size_t)info.imageSizePixels.y() * 4;
        size_t imageRowSize = 4 * info.imageSizePixels.x();
    
        SkImageInfo finalImgInfo = SkImageInfo::Make(info.imageSizePixels.x(), info.imageSizePixels.y(), kRGBA_8888_SkColorType, kUnpremul_SkAlphaType);
        std::vector<uint8_t> finalImgRawData(imageByteSize);
        SkPixmap finalImgData(finalImgInfo, finalImgRawData.data(), imageRowSize);
        
        sk_sp<SkSurface> surface = w->main.create_native_surface(w->main.window.size, true);
        SkCanvas* screenshotCanvas = surface->getCanvas();
        if(!screenshotCanvas) {
            Logger::get().log("INFO", "Screenshot Tool could not make canvas");
            return;
        }
    
        for(int i = 0; i < info.imageSizePixels.x(); i += w->main.window.size.x())
            for(int j = 0; j < info.imageSizePixels.y(); j += w->main.window.size.y())
                take_screenshot_area_hw(w, surface, screenshotCanvas, finalImgRawData.data(), info.imageSizePixels, Vector2i{i, j}, Vector2i{std::min(w->main.window.size.x(), info.imageSizePixels.x() - i), std::min(w->main.window.size.y(), info.imageSizePixels.y() - j)}, w->main.window.size, info.type != WorldScreenshotInfo::ScreenshotType::JPG && info.transparentBackground, info.cameraCoords, info.imageBounds, info.displayGrid);
    
        bool success = false;
        SkDynamicMemoryWStream out;
    
        switch(info.type) {
            case WorldScreenshotInfo::ScreenshotType::JPG:
                success = SkJpegEncoder::Encode(&out, finalImgData, {});
                break;
            case WorldScreenshotInfo::ScreenshotType::PNG:
                success = SkPngEncoder::Encode(&out, finalImgData, {});
                break;
            case WorldScreenshotInfo::ScreenshotType::WEBP:
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
            if(!SDL_SaveFile(info.filePath.string().c_str(), skData->bytes(), skData->size()))
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
        Vector2f canvasSize{info.imageBounds.max.x() - info.imageBounds.min.x(), info.imageBounds.max.y() - info.imageBounds.min.y()};
        SkRect canvasBounds = SkRect::MakeLTRB(0.0f, 0.0f, canvasSize.x(), canvasSize.y());
        std::unique_ptr<SkCanvas> canvas = SkSVGCanvas::Make(canvasBounds, &out, SkSVGCanvas::kConvertTextToPaths_Flag | SkSVGCanvas::kNoPrettyXML_Flag);
        take_screenshot_svg(w, canvas.get(), info.transparentBackground, info.cameraCoords, info.imageBounds);
        canvas = nullptr; // Ensure that SVG is completely written into the stream
        out.flush();

    #ifndef __EMSCRIPTEN__
        try {
            auto skData = out.detachAsData();
            if(!SDL_SaveFile(info.filePath.string().c_str(), skData->bytes(), skData->size()))
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

void take_screenshot_svg(const std::shared_ptr<World>& w, SkCanvas* canvas, bool transparentBackground, const CoordSpaceHelper& cameraCoords, const SCollision::AABB<float>& imageBounds) {
    float secRectX1 = imageBounds.min.x();
    float secRectX2 = imageBounds.max.x();
    float secRectY1 = imageBounds.min.y();
    float secRectY2 = imageBounds.max.y();

    Vector2f canvasSize{imageBounds.max.x() - imageBounds.min.x(), imageBounds.max.y() - imageBounds.min.y()};

    WorldVec topLeft = cameraCoords.from_space({secRectX1, secRectY1});
    WorldVec topRight = cameraCoords.from_space({secRectX2, secRectY1});
    WorldVec bottomLeft = cameraCoords.from_space({secRectX1, secRectY2});
    WorldVec bottomRight = cameraCoords.from_space({secRectX2, secRectY2});
    WorldVec camCenter = (topLeft + bottomRight) / WorldScalar(2);

    WorldVec vectorZoom;
    WorldScalar distX = (camCenter - (topLeft + bottomLeft) * WorldScalar(0.5)).norm();
    WorldScalar distY = (camCenter - (topLeft + topRight) * WorldScalar(0.5)).norm();
    vectorZoom.x() = distX / WorldScalar(canvasSize.x() * 0.5);
    vectorZoom.y() = distY / WorldScalar(canvasSize.y() * 0.5);
    WorldScalar newInverseScale = (vectorZoom.x() + vectorZoom.y()) * WorldScalar(0.5);

    DrawData screenshotDrawData = w->drawData;
    screenshotDrawData.cam.set_based_on_properties(*w, topLeft, newInverseScale, cameraCoords.rotation);
    screenshotDrawData.cam.set_viewing_area(canvasSize);
    screenshotDrawData.takingScreenshot = true;
    screenshotDrawData.transparentBackground = transparentBackground;
    screenshotDrawData.drawGrids = false;
    screenshotDrawData.isSVGRender = true;
    screenshotDrawData.refresh_draw_optimizing_values();
    w->main.draw_world(canvas, w->main.world, screenshotDrawData);
}

void take_screenshot_area_hw(const std::shared_ptr<World>& w, const sk_sp<SkSurface>& surface, SkCanvas* canvas, void* fullImgRawData, const Vector2i& fullImageSize, const Vector2i& sectionImagePos, const Vector2i& sectionImageSize, const Vector2i& canvasSize, bool transparentBackground, const CoordSpaceHelper& cameraCoords, const SCollision::AABB<float>& imageBounds, bool displayGrid) {
    float secRectX1 = imageBounds.min.x() + (imageBounds.max.x() - imageBounds.min.x()) * (sectionImagePos.x() / (double)fullImageSize.x());
    float secRectX2 = imageBounds.min.x() + (imageBounds.max.x() - imageBounds.min.x()) * ((sectionImagePos.x() + canvasSize.x()) / (double)fullImageSize.x());
    float secRectY1 = imageBounds.min.y() + (imageBounds.max.y() - imageBounds.min.y()) * (sectionImagePos.y() / (double)fullImageSize.y());
    float secRectY2 = imageBounds.min.y() + (imageBounds.max.y() - imageBounds.min.y()) * ((sectionImagePos.y() + canvasSize.y()) / (double)fullImageSize.y());

    WorldVec topLeft = cameraCoords.from_space({secRectX1, secRectY1});
    WorldVec topRight = cameraCoords.from_space({secRectX2, secRectY1});
    WorldVec bottomLeft = cameraCoords.from_space({secRectX1, secRectY2});
    WorldVec bottomRight = cameraCoords.from_space({secRectX2, secRectY2});
    WorldVec camCenter = (topLeft + bottomRight) / WorldScalar(2);

    WorldVec vectorZoom;
    WorldScalar distX = (camCenter - (topLeft + bottomLeft) * WorldScalar(0.5)).norm();
    WorldScalar distY = (camCenter - (topLeft + topRight) * WorldScalar(0.5)).norm();
    vectorZoom.x() = distX / WorldScalar(canvasSize.x() * 0.5);
    vectorZoom.y() = distY / WorldScalar(canvasSize.y() * 0.5);
    WorldScalar newInverseScale = (vectorZoom.x() + vectorZoom.y()) * WorldScalar(0.5);

    DrawData screenshotDrawData = w->drawData;
    screenshotDrawData.cam.set_based_on_properties(*w, topLeft, newInverseScale, cameraCoords.rotation);
    screenshotDrawData.cam.set_viewing_area(sectionImageSize.cast<float>());
    screenshotDrawData.takingScreenshot = true;
    screenshotDrawData.transparentBackground = transparentBackground;
    screenshotDrawData.drawGrids = displayGrid;
    screenshotDrawData.refresh_draw_optimizing_values();
    w->main.draw_world(canvas, w, screenshotDrawData);

    SkImageInfo aaImgInfo = SkImageInfo::Make(sectionImageSize.x(), sectionImageSize.y(), kRGBA_8888_SkColorType, kPremul_SkAlphaType);
    void* fullImgRawDataStartPt = (uint8_t*)fullImgRawData + 4 * (size_t)sectionImagePos.x() + 4 * (size_t)fullImageSize.x() * (size_t)sectionImagePos.y();
    SkPixmap aaImgData(aaImgInfo, fullImgRawDataStartPt, fullImageSize.x() * 4);
    if(!surface->readPixels(aaImgData, 0, 0))
        throw std::runtime_error("[ScreenshotTool::take_screenshot_area_hw] Error copy pixmap");
}
