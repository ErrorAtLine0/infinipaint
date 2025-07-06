#include "EraserTool.hpp"
#include "DrawingProgram.hpp"
#include "../Server/CommandList.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include "../SharedTypes.hpp"
#include <ranges>
#include "../DrawCollision.hpp"

EraserTool::EraserTool(DrawingProgram& initDrawP):
    drawP(initDrawP)
{
}

void EraserTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("eraser tool");
    t.gui.text_label_centered("Eraser");
    t.gui.slider_scalar_field("relwidth", "Size", &drawP.controls.relativeWidth, 3.0f, 40.0f);
    t.gui.pop_id();
}

void EraserTool::tool_update() {
    if(drawP.controls.cursorHoveringOverCanvas)
        drawP.world.main.input.hideCursor = true;

    if(drawP.controls.leftClickHeld) {
        const Vector2f& prevMousePos = drawP.controls.leftClick ? drawP.world.main.input.mouse.pos : drawP.world.main.input.mouse.lastPos;
        SCollision::ColliderCollection<float> cC;
        SCollision::generate_wide_line(cC, prevMousePos, drawP.world.main.input.mouse.pos, drawP.controls.relativeWidth * 2.0f, true);
        drawP.check_all_collisions_transform(cC);
        drawP.components.client_erase_if([&](uint64_t oldPlacement, const auto& c) {
            if(c->obj->globalCollisionCheck) {
                erasedComponents.emplace_back(oldPlacement, c->obj);
                DrawComponent::client_send_erase(drawP, c->id);
                return true;
            }
            return false;
        });
    }
    else if(!erasedComponents.empty()) {
        auto erasedCompVal = erasedComponents;
        drawP.world.undo.push(UndoManager::UndoRedoPair{
            [&, erasedCompVal]() {
                for(auto& comp : erasedCompVal)
                    if(drawP.components.get_id(comp.second) != ServerClientID{0, 0})
                        return false;

                for(auto& comp : erasedCompVal | std::views::reverse) {
                    drawP.components.client_insert(comp.first, comp.second);
                    comp.second->client_send_place(drawP, comp.first);
                }
                drawP.reset_tools();
                return true;
            },
            [&, erasedCompVal]() {
                for(auto& comp : erasedCompVal)
                    if(drawP.components.get_id(comp.second) == ServerClientID{0, 0})
                        return false;

                for(auto& comp : erasedCompVal) {
                    ServerClientID compID;
                    drawP.components.client_erase(comp.second, compID);
                    DrawComponent::client_send_erase(drawP, compID);
                }
                drawP.reset_tools();
                return true;
            }
        });
        erasedComponents.clear();
    }
}

void EraserTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(!drawData.main->toolbar.io->hoverObstructed) {
        SkColor4f c = drawP.world.main.canvasTheme.toolFrontColor;
        c.fA = 0.5f;

        SkPaint linePaint;
        linePaint.setColor4f(c);
        linePaint.setStyle(SkPaint::kStroke_Style);
        linePaint.setStrokeCap(SkPaint::kRound_Cap);
        linePaint.setStrokeWidth(drawP.controls.relativeWidth * 2.0f);
        SkPath erasePath;
        erasePath.moveTo(convert_vec2<SkPoint>(drawData.main->input.mouse.lastPos));
        erasePath.lineTo(convert_vec2<SkPoint>(drawData.main->input.mouse.pos));
        canvas->drawPath(erasePath, linePaint);
    }
}
