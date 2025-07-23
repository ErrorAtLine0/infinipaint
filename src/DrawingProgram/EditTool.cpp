#include "EditTool.hpp"
#include <chrono>
#include "DrawingProgram.hpp"
#include "../Server/CommandList.hpp"
#include "../MainProgram.hpp"
#include "../DrawData.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/SCollision.hpp"
#include "Helpers/Serializers.hpp"
#include "../DrawComponents/DrawRectangle.hpp"
#include "../DrawComponents/DrawTextBox.hpp"
#include "../InputManager.hpp"
#include "../SharedTypes.hpp"
#include <cereal/types/vector.hpp>
#include <memory>
#include <ranges>

EditTool::EditTool(DrawingProgram& initDrawP):
    drawP(initDrawP)
{
}

void EditTool::gui_toolbox() {
    Toolbar& t = drawP.world.main.toolbar;
    if(controls.isEditing) {
        std::shared_ptr<DrawComponent> a = controls.compToEdit.lock();
        bool editHappened = false;
        if(a) {
            switch(a->get_type()) {
                case DRAWCOMPONENT_TEXTBOX: {
                    editHappened = drawP.textBoxTool.edit_gui(std::static_pointer_cast<DrawTextBox>(a));
                    break;
                }
                case DRAWCOMPONENT_ELLIPSE: {
                    editHappened = drawP.ellipseDrawTool.edit_gui(std::static_pointer_cast<DrawEllipse>(a));
                    break;
                }
                case DRAWCOMPONENT_RECTANGLE: {
                    editHappened = drawP.rectDrawTool.edit_gui(std::static_pointer_cast<DrawRectangle>(a));
                    break;
                }
                case DRAWCOMPONENT_IMAGE: {
                    editHappened = drawP.imageTool.edit_gui(std::static_pointer_cast<DrawImage>(a));
                    break;
                }
                default: {
                    break;
                }
            }
        }
        if(editHappened) {
            a->temp_update(drawP);
            a->client_send_update_temp(drawP);
        }
    }
    else {
        t.gui.push_id("edit tool");
        t.gui.text_label_centered("Edit");
        t.gui.pop_id();
    }
}

void EditTool::add_point_handle(const HandleData& handle) {
    controls.pointHandles.emplace_back(handle);
}

void EditTool::reset_tool() {
    auto a = controls.compToEdit.lock();
    if(a && a->collabListInfo.lock()) {
        switch(a->get_type()) {
            case DRAWCOMPONENT_TEXTBOX:
                drawP.textBoxTool.commit_edit_updates(std::static_pointer_cast<DrawTextBox>(a), controls.prevData);
                break;
            case DRAWCOMPONENT_ELLIPSE:
                drawP.ellipseDrawTool.commit_edit_updates(std::static_pointer_cast<DrawEllipse>(a), controls.prevData);
                break;
            case DRAWCOMPONENT_RECTANGLE:
                drawP.rectDrawTool.commit_edit_updates(std::static_pointer_cast<DrawRectangle>(a), controls.prevData);
                break;
            case DRAWCOMPONENT_IMAGE:
                drawP.imageTool.commit_edit_updates(std::static_pointer_cast<DrawImage>(a), controls.prevData);
                break;
            default:
                break;
        }
        a->final_update(drawP);
        a->client_send_update_final(drawP);
    }
    controls.compToEdit.reset();
    controls.pointHandles.clear();
    controls.pointDragging = nullptr;
    controls.isEditing = false;
}

void EditTool::edit_start(const std::shared_ptr<DrawComponent>& comp) {
    controls.isEditing = true;
    switch(comp->get_type()) {
        case DRAWCOMPONENT_TEXTBOX: {
            drawP.textBoxTool.edit_start(std::static_pointer_cast<DrawTextBox>(comp), controls.prevData);
            break;
        }
        case DRAWCOMPONENT_ELLIPSE: {
            drawP.ellipseDrawTool.edit_start(std::static_pointer_cast<DrawEllipse>(comp), controls.prevData);
            break;
        }
        case DRAWCOMPONENT_RECTANGLE: {
            drawP.rectDrawTool.edit_start(std::static_pointer_cast<DrawRectangle>(comp), controls.prevData);
            break;
        }
        case DRAWCOMPONENT_IMAGE: {
            drawP.imageTool.edit_start(std::static_pointer_cast<DrawImage>(comp), controls.prevData);
            break;
        }
        default: {
            controls.isEditing = false;
            break;
        }
    }
    if(controls.isEditing) {
        controls.compToEdit = comp;
        comp->lastUpdateTime = std::chrono::steady_clock::now();
    }
}

void EditTool::tool_update() {
    using namespace SCollision;
    ColliderCollection<float> ci;
    ci.circle.emplace_back(Circle<float>{drawP.world.main.input.mouse.pos, 1.0f});
    ci.recalculate_bounds();

    if(!controls.isEditing) {
        if(drawP.controls.leftClick) {
            drawP.check_all_collisions_transform(ci);
            for(auto& comp : drawP.components.client_list() | std::views::reverse) {
                if(comp->obj->globalCollisionCheck) {
                    edit_start(comp->obj);
                    break;
                }
            }
        }
    }
    else {
        std::shared_ptr<DrawComponent> a = controls.compToEdit.lock();
        if(a) {
            bool isMovingPoint = false;
            bool clickedAway = false;
            if(!controls.pointDragging && drawP.controls.leftClick) {
                for(HandleData& h : controls.pointHandles) {
                    if(SCollision::collide(ci, Circle<float>(drawP.world.drawData.cam.c.to_space(a->coords.from_space(*h.p)), DRAG_POINT_RADIUS))) {
                        controls.pointDragging = &h;
                        isMovingPoint = true;
                        break;
                    }
                }
                if(!isMovingPoint && !a->collides_with_cam_coords(drawP.world.drawData.cam.c, ci, drawP.colliderAllocated))
                    clickedAway = true;
            }
            else if(controls.pointDragging) {
                if(drawP.controls.leftClickHeld) {
                    Vector2f newPos = a->coords.get_mouse_pos(drawP.world);
                    if(newPos != *controls.pointDragging->p) {
                        if(controls.pointDragging->min)
                            newPos = cwise_vec_max(*controls.pointDragging->min, newPos);
                        if(controls.pointDragging->max)
                            newPos = cwise_vec_min(*controls.pointDragging->max, newPos);
                        *controls.pointDragging->p = newPos;
                        a->temp_update(drawP);
                        a->client_send_update_temp(drawP);
                    }
                    isMovingPoint = true;
                }
                else
                    controls.pointDragging = nullptr;
            }

            a->lastUpdateTime = std::chrono::steady_clock::now();

            bool shouldNotReset;
            switch(a->get_type()) {
                case DRAWCOMPONENT_TEXTBOX: {
                    shouldNotReset = drawP.textBoxTool.edit_update(std::static_pointer_cast<DrawTextBox>(a));
                    break;
                }
                case DRAWCOMPONENT_ELLIPSE: {
                    shouldNotReset = drawP.ellipseDrawTool.edit_update(std::static_pointer_cast<DrawEllipse>(a));
                    break;
                }
                case DRAWCOMPONENT_RECTANGLE: {
                    shouldNotReset = drawP.rectDrawTool.edit_update(std::static_pointer_cast<DrawRectangle>(a));
                    break;
                }
                case DRAWCOMPONENT_IMAGE: {
                    shouldNotReset = drawP.imageTool.edit_update(std::static_pointer_cast<DrawImage>(a));
                    break;
                }
                default: {
                    shouldNotReset = false;
                    break;
                }
            }
            if(!shouldNotReset || clickedAway)
                reset_tool();
        }
        else
            reset_tool();
    }
}

void EditTool::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(controls.isEditing) {
        std::shared_ptr<DrawComponent> a = controls.compToEdit.lock();
        if(a) {
            for(HandleData& h : controls.pointHandles)
                drawP.draw_drag_circle(canvas, drawData.cam.c.to_space((a->coords.from_space(*h.p))), {0.1f, 0.9f, 0.9f, 1.0f}, drawData);
        }
    }
}
