#include "GridModifyTool.hpp"
#include "DrawingProgram.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include "Helpers/Networking/NetLibrary.hpp"
#include <Helpers/NetworkingObjects/NetObjWeakPtr.hpp>
#include <Helpers/NetworkingObjects/NetObjGenericSerializedClass.hpp>
#include <cstddef>

using namespace NetworkingObjects;

GridModifyTool::GridModifyTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{
}

void GridModifyTool::set_grid(const NetObjPtr<WorldGrid>& newGrid) {
    grid = newGrid;
    if(drawP.world.gridMan.grids->contains(grid)) {
        WorldGrid& g = *grid;
        oldGrid = g;
        CoordSpaceHelper newCam;
        newCam.inverseScale = g.size / WorldScalar(WorldGrid::GRID_UNIT_PIXEL_SIZE);
        newCam.pos = g.offset - drawP.world.main.window.size.cast<WorldScalar>() * newCam.inverseScale * WorldScalar(0.5);
        newCam.rotation = 0.0;
        drawP.world.drawData.cam.smooth_move_to(drawP.world, newCam, drawP.world.main.window.size.cast<float>());
    }
}

DrawingProgramToolType GridModifyTool::get_type() {
    return DrawingProgramToolType::GRIDMODIFY;
}

void GridModifyTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("Grid modify tool");
    t.gui.text_label_centered("Edit Grid");
    if(drawP.world.gridMan.grids->contains(grid)) {
        WorldGrid& g = *grid;
        t.gui.input_text_field("grid name", "Name", &g.name);
        t.gui.checkbox_field("Visible", "Visible", &g.visible);
        t.gui.checkbox_field("Display in Front", "Display in front of canvas", &g.displayInFront);
        size_t typeSelected = static_cast<size_t>(g.gridType);
        std::vector<std::string> listOfGridTypes = {
            "Circle Points",
            "Square Points",
            "Square Lines",
            "Horizontal Lines"
        };
        t.gui.left_to_right_line_layout([&]() {
            t.gui.text_label("Type");
            t.gui.dropdown_select("filepicker select type", &typeSelected, listOfGridTypes, 200.0f);
        });
        g.gridType = static_cast<WorldGrid::GridType>(typeSelected);
        uint32_t sDiv = g.subdivisions;
        t.gui.input_scalar_field<uint32_t>("Subdivisions", "Subdivisions", &sDiv, 1, 10);
        g.set_subdivisions(sDiv);
        bool divOut = g.removeDivisionsOutwards;
        t.gui.checkbox_field("Subdivide outwards", "Subdivide when zooming out", &divOut);
        g.set_remove_divisions_outwards(divOut);
        t.gui.left_to_right_line_layout([&]() {
            if(t.gui.color_button_big("Grid Color", &g.color, &g.color == t.colorRight))
                t.color_selector_right(&g.color == t.colorRight ? nullptr : &g.color);
            t.gui.text_label("Grid Color");
        });
        bool bounded = g.bounds.has_value();
        bool prevBoundedValue = bounded;
        t.gui.checkbox_field("Bounded", "Bounded", &bounded);
        if(bounded && !prevBoundedValue) {
            SCollision::AABB<WorldScalar> newBounds;
            newBounds.min = g.offset - drawP.world.drawData.cam.c.dir_from_space(drawP.world.main.window.size.cast<float>() * 0.3f);
            newBounds.max = g.offset + drawP.world.drawData.cam.c.dir_from_space(drawP.world.main.window.size.cast<float>() * 0.3f);
            g.bounds = newBounds;
        }
        else if(!bounded && prevBoundedValue)
            g.bounds = std::nullopt;
        t.gui.checkbox_field("Show Coordinates", "Show Coordinates (visible\nwhen canvas isn't rotated)", &g.showCoordinates);
    }
    t.gui.pop_id();
}

bool GridModifyTool::right_click_popup_gui(Vector2f popupPos) {
    Toolbar& t = drawP.world.main.toolbar;
    t.paint_popup(popupPos);
    return true;
}

void GridModifyTool::tool_update() {
    if(drawP.world.gridMan.grids->contains(grid)) {
        WorldGrid& g = *grid;
        switch(selectionMode) {
            case 0:
                if(drawP.controls.leftClick) {
                    Vector2f gOffsetScreenPos = drawP.world.drawData.cam.c.to_space(g.offset);
                    Vector2f gSizeScreenPos = drawP.world.drawData.cam.c.to_space(g.offset + WorldVec{g.size, 0});
                    if(SCollision::collide(SCollision::Circle(gOffsetScreenPos, drawP.drag_point_radius()), drawP.world.main.input.mouse.pos))
                        selectionMode = 1;
                    else if(SCollision::collide(SCollision::Circle(gSizeScreenPos, drawP.drag_point_radius()), drawP.world.main.input.mouse.pos))
                        selectionMode = 2;
                    else if(g.bounds.has_value()) {
                        const auto& b = g.bounds.value();
                        Vector2f bMin = drawP.world.drawData.cam.c.to_space(b.min);
                        Vector2f bMax = drawP.world.drawData.cam.c.to_space(b.max);
                        if(SCollision::collide(SCollision::Circle(bMin, drawP.drag_point_radius()), drawP.world.main.input.mouse.pos))
                            selectionMode = 3;
                        else if(SCollision::collide(SCollision::Circle(bMax, drawP.drag_point_radius()), drawP.world.main.input.mouse.pos))
                            selectionMode = 4;
                    }
                }
                break;
            case 1:
                if(drawP.controls.leftClickHeld)
                    g.offset = drawP.world.get_mouse_world_pos();
                else
                    selectionMode = 0;
                break;
            case 2:
                if(drawP.controls.leftClickHeld)
                    g.size = std::max(FixedPoint::abs(drawP.world.get_mouse_world_pos().x() - g.offset.x()), WorldScalar(1));
                else
                    selectionMode = 0;
                break;
            case 3:
                if(drawP.controls.leftClickHeld && g.bounds.has_value()) {
                    auto& b = g.bounds.value();
                    b.min = cwise_vec_min(drawP.world.get_mouse_world_pos(), b.max);
                }
                else
                    selectionMode = 0;
                break;
            case 4:
                if(drawP.controls.leftClickHeld && g.bounds.has_value()) {
                    auto& b = g.bounds.value();
                    b.max = cwise_vec_max(drawP.world.get_mouse_world_pos(), b.min);
                }
                else
                    selectionMode = 0;
                break;
        }
        drawP.world.delayedUpdateObjectManager.send_update_to_all<WorldGrid>(grid);
    }
    else
        selectionMode = 0;
}

bool GridModifyTool::prevent_undo_or_redo() {
    return false;
}

void GridModifyTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(drawP.world.gridMan.grids->contains(grid)) {
        WorldGrid& g = *grid;
        if(g.bounds.has_value()) {
            const auto& b = g.bounds.value();
            Vector2f bMin = drawP.world.drawData.cam.c.to_space(b.min);
            Vector2f bMax = drawP.world.drawData.cam.c.to_space(b.max);
            SkPaint rectBoundsPaint;
            rectBoundsPaint.setColor4f(drawP.world.canvasTheme.toolFrontColor);
            rectBoundsPaint.setStroke(true);
            rectBoundsPaint.setStrokeWidth(0.0f);
            canvas->drawRect(SkRect::MakeLTRB(bMin.x(), bMin.y(), bMax.x(), bMax.y()), rectBoundsPaint);
            if(selectionMode == 0 || selectionMode == 4)
                drawP.draw_drag_circle(canvas, bMax, {0.9f, 0.5f, 0.1f, 1.0f}, drawData);
            if(selectionMode == 0 || selectionMode == 3)
                drawP.draw_drag_circle(canvas, bMin, {0.9f, 0.5f, 0.1f, 1.0f}, drawData);
        }
        if(selectionMode == 0 || selectionMode == 2) {
            Vector2f gSizeScreenPos;
            if(drawP.world.get_mouse_world_pos().x() < g.offset.x() && selectionMode == 2)
                gSizeScreenPos = drawData.cam.c.to_space(g.offset - WorldVec{g.size, 0});
            else
                gSizeScreenPos = drawData.cam.c.to_space(g.offset + WorldVec{g.size, 0});
            drawP.draw_drag_circle(canvas, gSizeScreenPos, {0.1f, 0.9f, 0.9f, 1.0f}, drawData);
        }
        if(selectionMode == 0 || selectionMode == 1 || selectionMode == 3 || selectionMode == 4) {
            Vector2f gOffset = drawData.cam.c.to_space(g.offset);
            drawP.draw_drag_circle(canvas, gOffset, {1.0f, 0.27f, 0.27f, 1.0f}, drawData);
        }
    }
}

void GridModifyTool::switch_tool(DrawingProgramToolType newTool) {
    NetworkingObjects::generic_serialized_class_send_update_to_all<WorldGrid>(grid);
}
