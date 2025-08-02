#include "InkDropperTool.hpp"
#include "DrawingProgram.hpp"
#include "../Server/CommandList.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
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
#ifdef USE_SKIA_BACKEND_GRAPHITE
    #include <include/gpu/graphite/Surface.h>
#elif USE_SKIA_BACKEND_GANESH
    #include <include/gpu/ganesh/GrDirectContext.h>
    #include <include/gpu/ganesh/SkSurfaceGanesh.h>
#endif
#include <Helpers/Logger.hpp>

InkDropperTool::InkDropperTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{
}

DrawingProgramToolType InkDropperTool::get_type() {
    return DrawingProgramToolType::INKDROPPER;
}

void InkDropperTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("Color select tool");
    t.gui.text_label_centered("Color Select");
    t.gui.pop_id();
}

void InkDropperTool::tool_update() {
    if(drawP.controls.leftClick) {
        SkSurfaceProps props;
        SkImageInfo imgInfo = SkImageInfo::MakeN32Premul(drawP.world.main.window.size.x(), drawP.world.main.window.size.y());
        #ifdef USE_SKIA_BACKEND_GRAPHITE
            sk_sp<SkSurface> surface = SkSurfaces::RenderTarget(drawP.world.main.window.recorder(), imgInfo, skgpu::Mipmapped::kNo, &props);
        #elif USE_SKIA_BACKEND_GANESH
            sk_sp<SkSurface> surface = SkSurfaces::RenderTarget(drawP.world.main.window.ctx.get(), skgpu::Budgeted::kNo, imgInfo, 0, &props);
        #endif
        if(!surface) {
            Logger::get().log("INFO", "[SkSurfaces::WrapBackendRenderTarget] Ink Dropper Tool could not make surface");
            return;
        }
        SkCanvas* inkDropperCanvas = surface->getCanvas();
        if(!inkDropperCanvas) {
            Logger::get().log("INFO", "Ink Dropper Tool could not make canvas");
            return;
        }
        int xPos = std::clamp<int>(drawP.world.main.input.mouse.pos.x(), 0, drawP.world.main.window.size.x() - 1);
        int yPos = std::clamp<int>(drawP.world.main.input.mouse.pos.y(), 0, drawP.world.main.window.size.y() - 1);

        inkDropperCanvas->save();
        drawP.world.drawData.dontUseDrawProgCache = true;
        drawP.world.main.draw(inkDropperCanvas);
        drawP.world.drawData.dontUseDrawProgCache = false;
        inkDropperCanvas->restore();
        //drawP.world.main.window.ctx->flush();

        Vector4<uint8_t> readData;
        SkImageInfo readDataInfo = SkImageInfo::Make(1, 1, kRGBA_8888_SkColorType, kUnpremul_SkAlphaType);
        #ifdef USE_SKIA_BACKEND_GRAPHITE
            // NOT WORKING IN GRAPHITE YET
            //surface->makeImageSnapshot()->readPixels(drawP.world.main.window.ctx.get(), readDataInfo, (void*)readData.data(), 4, xPos, yPos);
        #elif USE_SKIA_BACKEND_GANESH
            surface->makeImageSnapshot()->readPixels(drawP.world.main.window.ctx.get(), readDataInfo, (void*)readData.data(), 4, xPos, yPos);
        #endif

        drawP.controls.foregroundColor = readData.cast<float>() / 255.0f;
    }
}

bool InkDropperTool::prevent_undo_or_redo() {
    return false;
}

void InkDropperTool::draw(SkCanvas* canvas, const DrawData& drawData) {
}

void InkDropperTool::reset_tool() {
}
