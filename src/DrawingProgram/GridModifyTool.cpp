#include "GridModifyTool.hpp"
#include "DrawingProgram.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"

GridModifyTool::GridModifyTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{
}

void GridModifyTool::set_grid_name(const std::string& gridToModifyName) {
    gridName = gridToModifyName;
    auto gridFoundIt = drawP.world.gridMan.grids.find(gridName);
    if(gridFoundIt != drawP.world.gridMan.grids.end()) {
        WorldGrid& g = gridFoundIt->second;
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
    auto gridFoundIt = drawP.world.gridMan.grids.find(gridName);
    if(gridFoundIt != drawP.world.gridMan.grids.end()) {
        WorldGrid& g = gridFoundIt->second;
        t.gui.text_label("Name: " + gridName);
        t.gui.checkbox_field("Visible", "Visible", &g.visible);
        size_t typeSelected = static_cast<size_t>(g.gridType);
        std::vector<std::string> listOfGridTypes = {
            "Circle Points",
            "Square Points",
            "Square Lines",
            "Ruled"
        };
        t.gui.left_to_right_line_layout([&]() {
            t.gui.text_label("Type");
            t.gui.dropdown_select("filepicker select type", &typeSelected, listOfGridTypes, 200.0f);
        });
        g.gridType = static_cast<WorldGrid::GridType>(typeSelected);
        uint32_t sDiv = g.get_subdivisions();
        t.gui.input_scalar_field<uint32_t>("Subdivisions", "Subdivisions", &sDiv, 0, 10);
        g.set_subdivisions(sDiv);
        bool divOut = g.get_remove_divisions_outwards();
        t.gui.checkbox_field("Subdivide outwards", "Subdivide outwards", &divOut);
        g.set_remove_divisions_outwards(divOut);
        t.gui.left_to_right_line_layout([&]() {
            CLAY({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(40), .height = CLAY_SIZING_FIXED(40)}}}) {
                if(t.gui.color_button("Grid Color", &g.color, &g.color == t.colorRight))
                    t.color_selector_right(&g.color == t.colorRight ? nullptr : &g.color);
            }
            t.gui.text_label("Grid Color");
        });
    }
    t.gui.pop_id();
}

void GridModifyTool::tool_update() {
    auto gridFoundIt = drawP.world.gridMan.grids.find(gridName);
    if(gridFoundIt != drawP.world.gridMan.grids.end()) {
        WorldGrid& g = gridFoundIt->second;
        switch(selectionMode) {
            case 0: {
                if(drawP.controls.leftClick) {
                    Vector2f gOffsetScreenPos = drawP.world.drawData.cam.c.to_space(g.offset);
                    Vector2f gSizeScreenPos = drawP.world.drawData.cam.c.to_space(g.offset + WorldVec{g.size, 0});
                    if(SCollision::collide(SCollision::Circle(gOffsetScreenPos, DRAG_POINT_RADIUS), drawP.world.main.input.mouse.pos))
                        selectionMode = 1;
                    else if(SCollision::collide(SCollision::Circle(gSizeScreenPos, DRAG_POINT_RADIUS), drawP.world.main.input.mouse.pos))
                        selectionMode = 2;
                }
                break;
            }
            case 1: {
                g.offset = drawP.world.get_mouse_world_pos();
                if(!drawP.controls.leftClickHeld)
                    selectionMode = 0;
                break;
            }
            case 2: {
                g.size = std::max(FixedPoint::abs(drawP.world.get_mouse_world_pos().x() - g.offset.x()), WorldScalar(1));
                if(!drawP.controls.leftClickHeld)
                    selectionMode = 0;
                break;
            }
        }
    }
    else
        selectionMode = 0;
}

bool GridModifyTool::prevent_undo_or_redo() {
    return false;
}

void GridModifyTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    auto gridFoundIt = drawP.world.gridMan.grids.find(gridName);
    if(gridFoundIt != drawP.world.gridMan.grids.end()) {
        WorldGrid& g = gridFoundIt->second;
        if(selectionMode == 0 || selectionMode == 2) {
            Vector2f gSizeScreenPos = drawData.cam.c.to_space(g.offset + WorldVec{g.size, 0});
            drawP.draw_drag_circle(canvas, gSizeScreenPos, {0.1f, 0.9f, 0.9f, 1.0f}, drawData);
        }
        if(selectionMode == 0 || selectionMode == 1) {
            Vector2f gOffset = drawData.cam.c.to_space(g.offset);
            drawP.draw_drag_circle(canvas, gOffset, {1.0f, 0.27f, 0.27f, 1.0f}, drawData);
        }
    }
}

void GridModifyTool::switch_tool(DrawingProgramToolType newTool) {
}
