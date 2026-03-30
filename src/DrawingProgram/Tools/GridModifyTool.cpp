#include "GridModifyTool.hpp"
#include "../DrawingProgram.hpp"
#include "../../MainProgram.hpp"
#include "../../DrawData.hpp"
#include "Helpers/Networking/NetLibrary.hpp"
#include "Helpers/NetworkingObjects/NetObjTemporaryPtr.decl.hpp"
#include <Helpers/NetworkingObjects/NetObjWeakPtr.hpp>
#include <Helpers/NetworkingObjects/NetObjGenericSerializedClass.hpp>
#include <cstddef>

#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/LayoutHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/TextBoxHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/CheckBoxHelpers.hpp"
#include "../../GUIStuff/Elements/DropDown.hpp"

using namespace NetworkingObjects;

GridModifyTool::GridModifyTool(DrawingProgram& initDrawP):
    DrawingProgramToolBase(initDrawP)
{
}

void GridModifyTool::set_grid(const NetObjWeakPtr<WorldGrid>& newGrid) {
    grid = newGrid;
    NetworkingObjects::NetObjTemporaryPtr<WorldGrid> gLock = grid.lock();
    if(gLock) {
        WorldGrid& g = *gLock;
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

void GridModifyTool::erase_component(CanvasComponentContainer::ObjInfo* erasedComp) {
}

void GridModifyTool::input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) {
    if(button.button == InputManager::MouseButton::LEFT) {
        switch(selectionMode) {
            case 0:
                if(button.down) {
                    NetworkingObjects::NetObjTemporaryPtr<WorldGrid> gLock = grid.lock();
                    if(gLock) {
                        WorldGrid& g = *gLock;
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
                }
                break;
            case 1:
            case 2:
            case 3:
            case 4:
                if(!button.down)
                    selectionMode = 0;
                break;
        }
    }
}

void GridModifyTool::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    NetworkingObjects::NetObjTemporaryPtr<WorldGrid> gLock = grid.lock();
    if(gLock) {
        WorldGrid& g = *gLock;
        auto mouseWorldPos = drawP.world.drawData.cam.c.from_space(motion.pos);
        switch(selectionMode) {
            case 0:
                break;
            case 1:
                g.offset = mouseWorldPos;
                break;
            case 2:
                g.size = std::max(FixedPoint::abs(mouseWorldPos.x() - g.offset.x()), WorldScalar(1));
                break;
            case 3:
                if(g.bounds.has_value()) {
                    auto& b = g.bounds.value();
                    b.min = cwise_vec_min(mouseWorldPos, b.max);
                }
                else
                    selectionMode = 0;
                break;
            case 4:
                if(g.bounds.has_value()) {
                    auto& b = g.bounds.value();
                    b.max = cwise_vec_max(mouseWorldPos, b.min);
                }
                else
                    selectionMode = 0;
                break;
        }
    }
}

void GridModifyTool::gui_toolbox() {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    Toolbar& t = drawP.world.main.toolbar;
    t.gui.new_id("Grid modify tool", [&] {
        text_label_centered(t.gui, "Edit Grid");
        NetworkingObjects::NetObjTemporaryPtr<WorldGrid> gLock = grid.lock();
        if(gLock) {
            WorldGrid& g = *gLock;
            input_text_field(t.gui, "grid name", "Name", &g.name);
            checkbox_boolean_field(t.gui, "Visible", "Visible", &g.visible);
            checkbox_boolean_field(t.gui, "Display in Front", "Display in front of canvas", &g.displayInFront);
            size_t typeSelected = static_cast<size_t>(g.gridType);
            std::vector<std::string> listOfGridTypes = {
                "Circle Points",
                "Square Points",
                "Square Lines",
                "Horizontal Lines"
            };
            left_to_right_line_layout(t.gui, [&]() {
                text_label(t.gui, "Type");
                t.gui.element<DropDown<size_t>>("filepicker select type", &typeSelected, listOfGridTypes);
            });
            g.gridType = static_cast<WorldGrid::GridType>(typeSelected);
            uint32_t sDiv = g.subdivisions;
            input_scalar_field<uint32_t>(t.gui, "Subdivisions", "Subdivisions", &sDiv, 1, 10);
            g.set_subdivisions(sDiv);
            bool divOut = g.removeDivisionsOutwards;
            checkbox_boolean_field(t.gui, "Subdivide outwards", "Subdivide when zooming out", &divOut);
            g.set_remove_divisions_outwards(divOut);
            left_to_right_line_layout(t.gui, [&]() {
                //if(t.gui.color_button_big("Grid Color", &g.color, &g.color == t.colorRight))
                //    t.color_selector_right(&g.color == t.colorRight ? nullptr : &g.color);
                text_label(t.gui, "Grid Color");
            });
            bool bounded = g.bounds.has_value();
            bool prevBoundedValue = bounded;
            checkbox_boolean_field(t.gui, "Bounded", "Bounded", &bounded);
            if(bounded && !prevBoundedValue) {
                SCollision::AABB<WorldScalar> newBounds;
                newBounds.min = g.offset - drawP.world.drawData.cam.c.dir_from_space(drawP.world.main.window.size.cast<float>() * 0.3f);
                newBounds.max = g.offset + drawP.world.drawData.cam.c.dir_from_space(drawP.world.main.window.size.cast<float>() * 0.3f);
                g.bounds = newBounds;
            }
            else if(!bounded && prevBoundedValue)
                g.bounds = std::nullopt;
            checkbox_boolean_field(t.gui, "Show Coordinates", "Show Coordinates (visible\nwhen canvas isn't rotated)", &g.showCoordinates);
        }
    });
}

void GridModifyTool::right_click_popup_gui(Vector2f popupPos) {
    Toolbar& t = drawP.world.main.toolbar;
    t.paint_popup(popupPos);
}

void GridModifyTool::tool_update() {
    NetworkingObjects::NetObjTemporaryPtr<WorldGrid> gLock = grid.lock();
    if(!gLock)
        selectionMode = 0;
}

bool GridModifyTool::prevent_undo_or_redo() {
    return true;
}

void GridModifyTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    NetworkingObjects::NetObjTemporaryPtr<WorldGrid> gLock = grid.lock();
    if(gLock) {
        WorldGrid& g = *gLock;
        if(g.bounds.has_value()) {
            const auto& b = g.bounds.value();
            Vector2f bMin = drawP.world.drawData.cam.c.to_space(b.min);
            Vector2f bMax = drawP.world.drawData.cam.c.to_space(b.max);
            SkPaint rectBoundsPaint;
            rectBoundsPaint.setColor4f(drawP.world.canvasTheme.get_tool_front_color());
            rectBoundsPaint.setStroke(true);
            rectBoundsPaint.setStrokeWidth(0.0f);
            rectBoundsPaint.setAntiAlias(drawData.skiaAA);
            canvas->drawRect(SkRect::MakeLTRB(bMin.x(), bMin.y(), bMax.x(), bMax.y()), rectBoundsPaint);
            if(selectionMode == 0 || selectionMode == 4)
                drawP.draw_drag_circle(canvas, bMax, {0.9f, 0.5f, 0.1f, 1.0f}, drawData);
            if(selectionMode == 0 || selectionMode == 3)
                drawP.draw_drag_circle(canvas, bMin, {0.9f, 0.5f, 0.1f, 1.0f}, drawData);
        }
        if(selectionMode == 0 || selectionMode == 2) {
            Vector2f gSizeScreenPos;
            auto mouseWorldPos = drawP.world.drawData.cam.c.from_space(drawData.main->input.mouse.pos);
            if(mouseWorldPos.x() < g.offset.x() && selectionMode == 2)
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
    NetworkingObjects::NetObjTemporaryPtr<WorldGrid> gLock = grid.lock();
    if(gLock)
        drawP.world.gridMan.finalize_grid_modify(gLock, oldGrid);
}
