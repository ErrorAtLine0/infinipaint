#include "DrawingProgram.hpp"
#include "Helpers/Networking/NetLibrary.hpp"
#include "Tools/DrawingProgramToolBase.hpp"
#include <include/core/SkPaint.h>
#include <include/core/SkVertices.h>
#include "../DrawCamera.hpp"
#include "Tools/EllipseDrawTool.hpp"
#include "Tools/EyeDropperTool.hpp"
#include "Tools/GridModifyTool.hpp"
#include "../InputManager.hpp"
#include "../SharedTypes.hpp"
#include "../CommandList.hpp"
#include <memory>
#include <optional>
#include <Helpers/ConvertVec.hpp>
#include <cereal/types/vector.hpp>
#include <include/core/SkImage.h>
#include <include/core/SkSurface.h>
#include "../World.hpp"
#include "../MainProgram.hpp"
#include <Helpers/FileDownloader.hpp>
#include <Helpers/NetworkingObjects/NetObjID.hpp>
#include <Helpers/StringHelpers.hpp>
#include "Tools/LassoSelectTool.hpp"
#include <Helpers/Logger.hpp>
#include <Helpers/Parallel.hpp>
#include <cereal/types/unordered_set.hpp>
#include <include/core/SkPathEffect.h>
#include <include/effects/SkDashPathEffect.h>
#include <chrono>
#include "../CanvasComponents/ImageCanvasComponent.hpp"
#include "Layers/DrawingProgramLayer.hpp"
#include "Layers/DrawingProgramLayerListItem.hpp"
#include <Helpers/Parallel.hpp>
#include "../ScaleUpCanvas.hpp"

#include "Tools/EraserTool.hpp"

#include "../GUIStuff/Elements/LayoutElement.hpp"
#include "../GUIStuff/Elements/RotateWheel.hpp"
#include "../GUIStuff/Elements/PositionAdjustingPopupMenu.hpp"
#include "../GUIStuff/ElementHelpers/ButtonHelpers.hpp"
#include "../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"

DrawingProgram::DrawingProgram(World& initWorld):
    world(initWorld),
    drawCache(*this),
    layerMan(*this),
    selection(*this)
{
    drawTool = DrawingProgramToolBase::allocate_tool_type(*this, DrawingProgramToolType::BRUSH);
}

void DrawingProgram::on_tab_out() {
    tempMoveToolSwitch = TemporaryMoveToolSwitch::NONE;
    selection.deselect_all();
    controls.leftClickHeld = false;
    controls.middleClickHeld = false;
}

void DrawingProgram::input_drop_file_callback(const InputManager::DropCallbackArgs& drop) {
    if(std::filesystem::is_regular_file(drop.data)) {
        #ifdef __EMSCRIPTEN_
            add_file_to_canvas_by_path(drop.data, world.main.input.mouse.pos, true);
        #else
            add_file_to_canvas_by_path(drop.data, drop.pos, true);
        #endif
    }
}

void DrawingProgram::input_drop_text_callback(const InputManager::DropCallbackArgs& drop) {
    if(is_valid_http_url(drop.data)) {
        CanvasComponentContainer* newContainer = new CanvasComponentContainer(world.netObjMan, CanvasComponentType::IMAGE);
        ImageCanvasComponent& img = static_cast<ImageCanvasComponent&>(newContainer->get_comp());
        newContainer->coords = world.drawData.cam.c;
        Vector2f imDim = Vector2f{100.0f, 100.0f};
        img.d.p1 = drop.pos - imDim;
        img.d.p2 = drop.pos + imDim;
        img.d.imageID = {0, 0};
        auto newObjInfo = layerMan.add_component_to_layer_being_edited(newContainer);
        layerMan.add_undo_place_component(newObjInfo);
        droppedDownloadingFiles.emplace_back(newObjInfo, world.main.window.size.cast<float>(), FileDownloader::download_data_from_url(drop.data));
    }
}

void DrawingProgram::input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button) {
    auto buttonCallbacks = [&](const InputManager::MouseButtonCallbackArgs& b) {
        drawTool->input_mouse_button_on_canvas_callback(b);
    };

    if(button.down) {
        if(button.button == InputManager::MouseButton::RIGHT) {
            if(!controls.middleClickHeld && !controls.leftClickHeld) {
                if(rightClickPopupLocation.has_value())
                    clear_right_click_popup();
                else
                    set_right_click_popup_location(world.main.input.mouse.pos / world.main.g.final_gui_scale());
            }
        }
        else {
            if(!rightClickPopupHoverOnClick)
                clear_right_click_popup();
            if(!world.main.g.gui.cursor_obstructed()) {
                if(button.button == InputManager::MouseButton::LEFT && !controls.middleClickHeld) {
                    controls.leftClickHeld = true;
                    buttonCallbacks(button);
                }
                else if(button.button == InputManager::MouseButton::MIDDLE) {
                    if(controls.leftClickHeld) {
                        controls.leftClickHeld = false;
                        InputManager::MouseButtonCallbackArgs leftReleaseCallback;
                        leftReleaseCallback.clicks = 0;
                        leftReleaseCallback.down = false;
                        leftReleaseCallback.pos = button.pos;
                        leftReleaseCallback.button = InputManager::MouseButton::LEFT;
                        buttonCallbacks(leftReleaseCallback);
                    }
                    controls.middleClickHeld = true;
                    buttonCallbacks(button);
                }
            }
        }
    }
    else if(!button.down) {
        if(controls.leftClickHeld && button.button == InputManager::MouseButton::LEFT) {
            controls.leftClickHeld = false;
            buttonCallbacks(button);
        }
        else if(controls.middleClickHeld && button.button == InputManager::MouseButton::MIDDLE) {
            controls.middleClickHeld = false;
            buttonCallbacks(button);
        }
    }

    if(toolToSwitchToAfterUpdate) {
        switch_to_tool_ptr(std::move(toolToSwitchToAfterUpdate));
        toolToSwitchToAfterUpdate = nullptr;
    }
}

void DrawingProgram::input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) {
    drawTool->input_mouse_motion_callback(motion);
}

void DrawingProgram::input_key_callback(const InputManager::KeyCallbackArgs& key) {
    switch(key.key) {
        case InputManager::KEY_DRAW_TOOL_BRUSH: {
            if(key.down && !key.repeat)
                switch_to_tool(DrawingProgramToolType::BRUSH);
            break;
        }
        case InputManager::KEY_DRAW_TOOL_ERASER: {
            if(key.down && !key.repeat)
                switch_to_tool(DrawingProgramToolType::ERASER);
            break;
        }
        case InputManager::KEY_DRAW_TOOL_RECTSELECT: {
            if(key.down && !key.repeat)
                switch_to_tool(DrawingProgramToolType::RECTSELECT);
            break;
        }
        case InputManager::KEY_DRAW_TOOL_RECTANGLE: {
            if(key.down && !key.repeat)
                switch_to_tool(DrawingProgramToolType::RECTANGLE);
            break;
        }
        case InputManager::KEY_DRAW_TOOL_ELLIPSE: {
            if(key.down && !key.repeat)
                switch_to_tool(DrawingProgramToolType::ELLIPSE);
            break;
        }
        case InputManager::KEY_DRAW_TOOL_TEXTBOX: {
            if(key.down && !key.repeat)
                switch_to_tool(DrawingProgramToolType::TEXTBOX);
            break;
        }
        case InputManager::KEY_DRAW_TOOL_EYEDROPPER: {
            if(key.down && !key.repeat)
                switch_to_tool(DrawingProgramToolType::EYEDROPPER);
            break;
        }
        case InputManager::KEY_DRAW_TOOL_SCREENSHOT: {
            if(key.down && !key.repeat)
                switch_to_tool(DrawingProgramToolType::SCREENSHOT);
            break;
        }
        case InputManager::KEY_DRAW_TOOL_ZOOM: {
            if(key.down && !key.repeat)
                switch_to_tool(DrawingProgramToolType::ZOOM);
            break;
        }
        case InputManager::KEY_DRAW_TOOL_LASSOSELECT: {
            if(key.down && !key.repeat)
                switch_to_tool(DrawingProgramToolType::LASSOSELECT);
            break;
        }
        case InputManager::KEY_DRAW_TOOL_PAN: {
            if(key.down && !key.repeat)
                switch_to_tool(DrawingProgramToolType::PAN);
            break;
        }
        case InputManager::KEY_DRAW_TOOL_LINE: {
            if(key.down && !key.repeat)
                switch_to_tool(DrawingProgramToolType::LINE);
            break;
        }
        case InputManager::KEY_HOLD_TO_PAN: {
            if(key.down && !key.repeat && tempMoveToolSwitch == TemporaryMoveToolSwitch::NONE) {
                toolTypeAfterTempMove = drawTool->get_type();
                switch_to_tool(DrawingProgramToolType::PAN);
                tempMoveToolSwitch = TemporaryMoveToolSwitch::PAN;
            }
            else if(!key.down && tempMoveToolSwitch == TemporaryMoveToolSwitch::PAN) {
                switch_to_tool(toolTypeAfterTempMove);
                tempMoveToolSwitch = TemporaryMoveToolSwitch::NONE;
                pen_tool_switch_check();
            }
            break;
        }
        case InputManager::KEY_HOLD_TO_ZOOM: {
            if(key.down && !key.repeat && tempMoveToolSwitch == TemporaryMoveToolSwitch::NONE) {
                toolTypeAfterTempMove = drawTool->get_type();
                switch_to_tool(DrawingProgramToolType::ZOOM);
                tempMoveToolSwitch = TemporaryMoveToolSwitch::ZOOM;
            }
            else if(!key.down && tempMoveToolSwitch == TemporaryMoveToolSwitch::ZOOM) {
                switch_to_tool(toolTypeAfterTempMove);
                tempMoveToolSwitch = TemporaryMoveToolSwitch::NONE;
                pen_tool_switch_check();
            }
            break;
        }
    }
    selection.input_key_callback_display_selection(key);
    drawTool->input_key_callback(key);
}

void DrawingProgram::input_pen_button_callback(const InputManager::PenButtonCallbackArgs& button) {
    pen_tool_switch_check();
    drawTool->input_pen_button_callback(button);
}

void DrawingProgram::input_pen_touch_callback(const InputManager::PenTouchCallbackArgs& touch) {
    pen_tool_switch_check();
    drawTool->input_pen_touch_callback(touch);
}

void DrawingProgram::input_pen_motion_callback(const InputManager::PenMotionCallbackArgs& motion) {
    pen_tool_switch_check();
    drawTool->input_pen_motion_callback(motion);
}

void DrawingProgram::input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis) {
    pen_tool_switch_check();
    drawTool->input_pen_axis_callback(axis);
}

void DrawingProgram::server_init_no_file() {
    layerMan.server_init_no_file();
}

void DrawingProgram::scale_up(const WorldScalar& scaleUpAmount) {
    selection.deselect_all();
    layerMan.scale_up(scaleUpAmount);
    if(controls.lockedCameraScale.has_value())
        controls.lockedCameraScale.value() *= scaleUpAmount;
    rebuild_cache();
    switch_to_tool(drawTool->get_type() == DrawingProgramToolType::GRIDMODIFY ? DrawingProgramToolType::EDIT : drawTool->get_type(), true);
}

void DrawingProgram::write_components_server(cereal::PortableBinaryOutputArchive& a) {
    layerMan.write_components_server(a);
}

void DrawingProgram::read_components_client(cereal::PortableBinaryInputArchive& a) {
    layerMan.read_components_client(a);
    drawCache.build({});
}

void DrawingProgram::init_server_callbacks() {
    world.netServer->add_recv_callback(SERVER_TRANSFORM_MANY_COMPONENTS, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        std::vector<std::pair<NetworkingObjects::NetObjID, CoordSpaceHelper>> transforms;
        message(transforms);

        auto clientData = world.netObjMan.get_obj_temporary_ref_from_id<ClientData>(NetworkingObjects::NetObjID(client->customID));
        if(clientData->get_grid_size() < world.ownClientData->get_grid_size()) {
            WorldScalar scaleUpAmount = get_canvas_scale_up_amount(world.ownClientData->get_grid_size(), clientData->get_grid_size());
            for(auto& [netID, coord] : transforms)
                coord.scale_about(WorldVec{0, 0}, scaleUpAmount, true);
        }
        
        process_transform_message(transforms);
        world.netServer->send_items_to_all_clients(RELIABLE_COMMAND_CHANNEL, CLIENT_TRANSFORM_MANY_COMPONENTS, transforms);
    });
}

void DrawingProgram::init_client_callbacks() {
    world.netClient->add_recv_callback(CLIENT_TRANSFORM_MANY_COMPONENTS, [&](cereal::PortableBinaryInputArchive& message) {
        std::vector<std::pair<NetworkingObjects::NetObjID, CoordSpaceHelper>> transforms;
        message(transforms);
        process_transform_message(transforms);
    });
}

void DrawingProgram::process_transform_message(const std::vector<std::pair<NetworkingObjects::NetObjID, CoordSpaceHelper>>& transforms) {
    auto selectedSet = selection.get_selection_as_set();
    for(auto& [id, coords] : transforms) {
        auto objPtr = world.netObjMan.get_obj_temporary_ref_from_id<CanvasComponentContainer>(id);
        if(!objPtr || selectedSet.contains(&(*objPtr->objInfo))) // Whatever transformation we're doing right now will overwrite this transformation, so we can ignore this message
            continue;
        if(objPtr->coords == coords)
            continue;
        objPtr->coords = coords;
        objPtr->commit_transform(*this);
    }
}

void DrawingProgram::send_transforms_for(const std::vector<CanvasComponentContainer::ObjInfo*>& objsToSendTransformsFor) {
    if(world.netObjMan.is_connected()) {
        std::vector<std::pair<NetworkingObjects::NetObjID, CoordSpaceHelper>> transforms;
        for(auto& obj : objsToSendTransformsFor)
            transforms.emplace_back(obj->obj.get_net_id(), obj->obj->coords);
        if(world.netObjMan.is_server())
            world.netServer->send_items_to_all_clients(RELIABLE_COMMAND_CHANNEL, CLIENT_TRANSFORM_MANY_COMPONENTS, transforms);
        else
            world.netClient->send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_TRANSFORM_MANY_COMPONENTS, transforms);
    }
}

void DrawingProgram::toolbar_gui() {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    GUIManager& gui = world.main.g.gui;
    auto& io = gui.io;

    gui.new_id("Drawing Program Toolbar GUI", [&] {
        gui.element<LayoutElement>("Drawing Program Toolbar GUI", [&] (const Clay_ElementId& lId) {
            CLAY(lId, {
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0)},
                    .padding = CLAY_PADDING_ALL(static_cast<uint16_t>(io.theme->padding1 / 2)),
                    .childGap = static_cast<uint16_t>(io.theme->childGap1 / 2), 
                    .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
                .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1)
            }) {

                auto tool_button = [&](const char* id, const std::string& svgPath, DrawingProgramToolType toolType) {
                    svg_icon_button(gui, id, svgPath, {
                        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                        .isSelected = drawTool->get_type() == toolType,
                        .onClick = [&, toolType] {
                            switch_to_tool(toolType);
                        }
                    });
                };

                tool_button("Brush Toolbar Button", "data/icons/brush.svg", DrawingProgramToolType::BRUSH);
                tool_button("Eraser Toolbar Button", "data/icons/eraser.svg", DrawingProgramToolType::ERASER);
                tool_button("Line Toolbar Button", "data/icons/line.svg", DrawingProgramToolType::LINE);
                tool_button("Text Toolbar Button", "data/icons/text.svg", DrawingProgramToolType::TEXTBOX);
                tool_button("Ellipse Toolbar Button", "data/icons/circle.svg", DrawingProgramToolType::ELLIPSE);
                tool_button("Rect Toolbar Button", "data/icons/rectangle.svg", DrawingProgramToolType::RECTANGLE);
                tool_button("RectSelect Toolbar Button", "data/icons/rectselect.svg", DrawingProgramToolType::RECTSELECT);
                tool_button("LassoSelect Toolbar Button", "data/icons/lassoselect.svg", DrawingProgramToolType::LASSOSELECT);
                tool_button("Edit Toolbar Button", "data/icons/cursor.svg", DrawingProgramToolType::EDIT);
                tool_button("Eyedropper Toolbar Button", "data/icons/eyedropper.svg", DrawingProgramToolType::EYEDROPPER);
                tool_button("Zoom Canvas Toolbar Button", "data/icons/zoom.svg", DrawingProgramToolType::ZOOM);
                tool_button("Pan Canvas Toolbar Button", "data/icons/hand.svg", DrawingProgramToolType::PAN);

                std::shared_ptr<double> newRotationAngle = std::make_shared<double>(world.drawData.cam.c.rotation);
                gui.element<RotateWheel>("Canvas Rotate Wheel", newRotationAngle.get(), [&, newRotationAngle] {
                    world.drawData.cam.c.rotate_about(world.drawData.cam.c.from_space(world.main.window.size.cast<float>() * 0.5f), *newRotationAngle - world.drawData.cam.c.rotation);
                    *newRotationAngle = world.drawData.cam.c.rotation;
                });
            }
        });
    });
}

void DrawingProgram::right_click_popup_gui() {
    if(rightClickPopupLocation.has_value())
        drawTool->right_click_popup_gui(rightClickPopupLocation.value());
}

void DrawingProgram::set_right_click_popup_location(const Vector2f& newLoc) {
    if(!rightClickPopupLocation.has_value() || rightClickPopupLocation.value() != newLoc) {
        rightClickPopupLocation = newLoc;
        rightClickPopupHoverOnClick = false;
        world.main.g.gui.set_to_layout();
    }
}

void DrawingProgram::clear_right_click_popup() {
    if(rightClickPopupLocation.has_value()) {
        rightClickPopupLocation = std::nullopt;
        world.main.g.gui.set_to_layout();
        rightClickPopupHoverOnClick = false;
    }
}

void DrawingProgram::popup_menu_action_button(const char* id, const char* text, const std::function<void()>& onClick) {
    GUIStuff::GUIManager& gui = world.main.g.gui;

    GUIStuff::ElementHelpers::text_button(gui, id, text, {
        .drawType = GUIStuff::SelectableButton::DrawType::TRANSPARENT_ALL,
        .wide = true,
        .centered = false,
        .onClick = onClick
    });
}

// Should only be used in selection allowing tools
void DrawingProgram::selection_action_menu(Vector2f popupPos) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    GUIStuff::GUIManager& gui = world.main.g.gui;

    gui.set_z_index(-1, [&] {
        gui.element<PositionAdjustingPopupMenu>("Selection popup menu", popupPos, [&] {
            text_label_light(gui, "Selection menu");
            popup_menu_action_button("Paste", "Paste", [&, popupPos] {
                selection.deselect_all();
                selection.paste_clipboard(popupPos * world.main.g.final_gui_scale());
            });
            popup_menu_action_button("Paste Image", "Paste Image", [&, popupPos] {
                selection.deselect_all();
                selection.paste_image(popupPos * world.main.g.final_gui_scale());
            });
            if(selection.is_something_selected()) {
                popup_menu_action_button("Copy", "Copy", [&] {
                    selection.selection_to_clipboard();
                });
                popup_menu_action_button("Cut", "Cut", [&] {
                    selection.selection_to_clipboard();
                    selection.delete_all();
                });
                popup_menu_action_button("Delete", "Delete", [&] {
                    selection.delete_all();
                });
                popup_menu_action_button("Bring to front of layer", "Bring to front of layer", [&] {
                    selection.push_selection_to_front();
                });
                popup_menu_action_button("Send to back of layer", "Send to back of layer", [&] {
                    selection.push_selection_to_back();
                });
            }
        });
    });
}

void DrawingProgram::right_click_popup_gui(Vector2f popupPos) {
    GUIStuff::GUIManager& gui = world.main.g.gui;

    gui.new_id("Drawing Program right click GUI", [&] {
        drawTool->right_click_popup_gui(popupPos);
    });
}

void DrawingProgram::tool_options_gui() {
    using namespace GUIStuff;

    GUIStuff::GUIManager& gui = world.main.g.gui;
    auto& io = gui.io;

    float minGUIWidth = drawTool->get_type() == DrawingProgramToolType::SCREENSHOT ? 300 : 200;
    gui.element<LayoutElement>("Drawing program tool options gui", [&] (const Clay_ElementId& lId) {
        CLAY(lId, {
            .layout = {
                .sizing = {.width = CLAY_SIZING_FIT(minGUIWidth), .height = CLAY_SIZING_FIT(0)},
                .padding = CLAY_PADDING_ALL(io.theme->padding1),
                .childGap = io.theme->childGap1,
                .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                .layoutDirection = CLAY_TOP_TO_BOTTOM
            },
            .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
            .cornerRadius = CLAY_CORNER_RADIUS(io.theme->windowCorners1)
        }) {
            drawTool->gui_toolbox();
        }
    });
}

void DrawingProgram::modify_grid(const NetworkingObjects::NetObjWeakPtr<WorldGrid>& gridToModify) {
    std::unique_ptr<GridModifyTool> newTool(std::make_unique<GridModifyTool>(*this));
    newTool->set_grid(gridToModify);
    switch_to_tool_ptr(std::move(newTool));
}

void DrawingProgram::update() {
    if(addFileInNextFrame) {
        add_file_to_canvas_by_path_execute(addFileInfo.first, addFileInfo.second);
        addFileInNextFrame = false;
    }

    selection.update();
    drawTool->tool_update();

    update_downloading_dropped_files();
    check_updateable_components();

    if(drawCache.check_rebuild_needed_from_framerate() || drawCache.should_rebuild())
        rebuild_cache();
}

void DrawingProgram::pen_tool_switch_check() {
    if(world.main.input.pen.isEraser && !temporaryEraser && tempMoveToolSwitch == TemporaryMoveToolSwitch::NONE) {
        if(drawTool->get_type() == DrawingProgramToolType::BRUSH)
            switch_to_tool(DrawingProgramToolType::ERASER);
        temporaryEraser = true;
    }
    else if(!world.main.input.pen.isEraser && temporaryEraser && tempMoveToolSwitch == TemporaryMoveToolSwitch::NONE) {
        if(drawTool->get_type() == DrawingProgramToolType::ERASER)
            switch_to_tool(DrawingProgramToolType::BRUSH);
        temporaryEraser = false;
    }
}

void DrawingProgram::invalidate_cache_at_component(CanvasComponentContainer::ObjInfo* objToCheck) {
    if(!selection.is_selected(objToCheck))
        drawCache.invalidate_cache_at_aabb(objToCheck->obj->get_world_bounds());
}

void DrawingProgram::preupdate_component(CanvasComponentContainer::ObjInfo* objToCheck) {
    if(!selection.is_selected(objToCheck))
        drawCache.preupdate_component(objToCheck);
}

void DrawingProgram::check_updateable_components() {
    for(auto& comp : updateableComponents)
        comp->obj->get_comp().update(*this);
}

bool DrawingProgram::is_actual_selection_tool(DrawingProgramToolType typeToCheck) {
    return typeToCheck == DrawingProgramToolType::RECTSELECT || typeToCheck == DrawingProgramToolType::LASSOSELECT || typeToCheck == DrawingProgramToolType::EDIT;
}

bool DrawingProgram::is_selection_allowing_tool(DrawingProgramToolType typeToCheck) {
    return is_actual_selection_tool(typeToCheck) || typeToCheck == DrawingProgramToolType::PAN || typeToCheck == DrawingProgramToolType::ZOOM;
}

void DrawingProgram::switch_to_tool_ptr(std::unique_ptr<DrawingProgramToolBase> newTool) {
    drawTool->switch_tool(newTool->get_type());
    drawTool = std::move(newTool);
    clear_right_click_popup();
    world.main.g.gui.set_to_layout();
}

void DrawingProgram::switch_to_tool(DrawingProgramToolType newToolType, bool force) {
    if(newToolType != drawTool->get_type() || force) {
        drawTool->switch_tool(newToolType);
        drawTool = DrawingProgramToolBase::allocate_tool_type(*this, newToolType);
        clear_right_click_popup();
        world.main.g.gui.set_to_layout();
    }
}

bool DrawingProgram::prevent_undo_or_redo() {
    return drawTool->prevent_undo_or_redo();
}

SkPaint DrawingProgram::select_tool_line_paint(const DrawData& drawData) {
    SkScalar intervals[] = {10, 5};
    float timeSinceEpoch = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now().time_since_epoch()).count();
    sk_sp<SkPathEffect> lassoLineDashEffect = SkDashPathEffect::Make({intervals, 2}, -std::fmod(timeSinceEpoch * 50, 15));

    SkPaint selectLinePaint;
    selectLinePaint.setStyle(SkPaint::kStroke_Style);
    selectLinePaint.setStrokeWidth(3);
    selectLinePaint.setColor4f(world.canvasTheme.get_tool_front_color());
    selectLinePaint.setPathEffect(lassoLineDashEffect);
    selectLinePaint.setBlender(CanvasTheme::get_visible_blend_mode());
    selectLinePaint.setAntiAlias(drawData.skiaAA);

    return selectLinePaint;
}

void DrawingProgram::rebuild_cache() {
    if(drawTool->get_type() == DrawingProgramToolType::ERASER) {
        EraserTool* eraserTool = static_cast<EraserTool*>(drawTool.get());
        drawCache.build(eraserTool->erasedComponents);
    }
    else if(is_selection_allowing_tool(drawTool->get_type()))
        drawCache.build(selection.get_selection_as_set());
    else
        drawCache.build({});
}

void DrawingProgram::update_downloading_dropped_files() {
    std::erase_if(droppedDownloadingFiles, [&](auto& downFile) {
        switch(downFile.downData->status) {
            case FileDownloader::DownloadData::Status::SUCCESS: {
                ImageCanvasComponent& img = static_cast<ImageCanvasComponent&>(downFile.comp->obj->get_comp());
                Vector2f dropPos = (img.d.p1 + img.d.p2) * 0.5f;

                ResourceData newResource;
                newResource.data = std::make_shared<std::string>(downFile.downData->str);
                newResource.name = downFile.downData->fileName;
                NetworkingObjects::NetObjID imageID = world.rMan.add_resource(newResource).get_net_id();
                ResourceDisplay* display = world.rMan.get_display_data(imageID);
                if(display->get_type() == ResourceDisplay::Type::FILE) {
                    Logger::get().log("WORLDFATAL", "Failed to parse image from URL");
                    auto& parentLayerComponents = downFile.comp->obj->parentLayer->get_layer().components;
                    parentLayerComponents->erase(parentLayerComponents, downFile.comp->obj->objInfo);
                }
                else {
                    Vector2f imTrueDim = display->get_dimensions();

                    float imWidth = imTrueDim.x() / (imTrueDim.x() + imTrueDim.y());
                    float imHeight = imTrueDim.y() / (imTrueDim.x() + imTrueDim.y());
                    Vector2f imDim = Vector2f{downFile.windowSizeWhenDropped.x() * imWidth, downFile.windowSizeWhenDropped.x() * imHeight} * display->get_dimension_scale();
                    img.d.p1 = dropPos - imDim;
                    img.d.p2 = dropPos + imDim;
                    img.d.imageID = imageID;
                    downFile.comp->obj->send_comp_update(*this, true);
                    downFile.comp->obj->commit_update(*this);
                }
                return true;
            }
            case FileDownloader::DownloadData::Status::FAILURE: {
                Logger::get().log("WORLDFATAL", "Failed to download data from URL");
                auto& parentLayerComponents = downFile.comp->obj->parentLayer->get_layer().components;
                parentLayerComponents->erase(parentLayerComponents, downFile.comp->obj->objInfo);
                return true;
            }
            case FileDownloader::DownloadData::Status::IN_PROGRESS:
                return false;
        }
        return false;
    });
}

void DrawingProgram::add_file_to_canvas_by_path(const std::filesystem::path& filePath, Vector2f dropPos, bool addInSameThread) {
    if(addInSameThread)
        add_file_to_canvas_by_path_execute(filePath, dropPos);
    else if(!addFileInNextFrame) {
        addFileInfo = {filePath, dropPos};
        addFileInNextFrame = true;
    }
}

void DrawingProgram::add_file_to_canvas_by_path_execute(const std::filesystem::path& filePath, Vector2f dropPos) {
    if(layerMan.is_a_layer_being_edited()) {
        NetworkingObjects::NetObjTemporaryPtr<ResourceData> imageTempPtr = world.rMan.add_resource_file(filePath);
        if(imageTempPtr) {
            NetworkingObjects::NetObjID imageID = imageTempPtr.get_net_id();
            ResourceDisplay* display = world.rMan.get_display_data(imageID);
            Vector2f imTrueDim = display->get_dimensions();
            CanvasComponentContainer* newContainer = new CanvasComponentContainer(world.netObjMan, CanvasComponentType::IMAGE);
            ImageCanvasComponent& img = static_cast<ImageCanvasComponent&>(newContainer->get_comp());
            newContainer->coords = world.drawData.cam.c;
            float imWidth = imTrueDim.x() / (imTrueDim.x() + imTrueDim.y());
            float imHeight = imTrueDim.y() / (imTrueDim.x() + imTrueDim.y());
            Vector2f imDim = Vector2f{world.main.window.size.x() * imWidth, world.main.window.size.x() * imHeight} * display->get_dimension_scale();
            img.d.p1 = dropPos - imDim;
            img.d.p2 = dropPos + imDim;
            img.d.imageID = imageID;
            auto newObjInfo = layerMan.add_component_to_layer_being_edited(newContainer);
            layerMan.add_undo_place_component(newObjInfo);
        }
    }
}

CanvasComponentContainer::ObjInfo* DrawingProgram::add_file_to_canvas_by_data(const std::string& fileName, std::string_view fileBuffer, Vector2f dropPos) {
    if(layerMan.is_a_layer_being_edited()) {
        ResourceData newResource;
        newResource.data = std::make_shared<std::string>(fileBuffer);
        newResource.name = fileName;
        NetworkingObjects::NetObjID imageID = world.rMan.add_resource(newResource).get_net_id();
        ResourceDisplay* display = world.rMan.get_display_data(imageID);
        Vector2f imTrueDim = display->get_dimensions();
        CanvasComponentContainer* newContainer = new CanvasComponentContainer(world.netObjMan, CanvasComponentType::IMAGE);
        ImageCanvasComponent& img = static_cast<ImageCanvasComponent&>(newContainer->get_comp());
        newContainer->coords = world.drawData.cam.c;
        float imWidth = imTrueDim.x() / (imTrueDim.x() + imTrueDim.y());
        float imHeight = imTrueDim.y() / (imTrueDim.x() + imTrueDim.y());
        Vector2f imDim = Vector2f{world.main.window.size.x() * imWidth, world.main.window.size.x() * imHeight} * display->get_dimension_scale();
        img.d.p1 = dropPos - imDim;
        img.d.p2 = dropPos + imDim;
        img.d.imageID = imageID;
        CanvasComponentContainer::ObjInfo* newObjInfo = layerMan.add_component_to_layer_being_edited(newContainer);
        layerMan.add_undo_place_component(newObjInfo);
        return newObjInfo;
    }
    return nullptr;
}

float DrawingProgram::drag_point_radius() {
    return 8.0f * world.main.g.final_gui_scale();
}

void DrawingProgram::draw_drag_circle(SkCanvas* canvas, const Vector2f& sPos, const SkColor4f& c, const DrawData& drawData, float radiusMultiplier) {
    float constantRadius = drag_point_radius() * radiusMultiplier;
    float constantThickness = constantRadius * 0.2f;
    canvas->drawCircle(sPos.x(), sPos.y(), constantRadius, SkPaint(c));
    SkPaint paintOutline(SkColor4f{0.95f, 0.95f, 0.95f, 1.0f});
    paintOutline.setStroke(true);
    paintOutline.setStrokeWidth(constantThickness);
    paintOutline.setAntiAlias(drawData.skiaAA);
    canvas->drawCircle(sPos.x(), sPos.y(), constantRadius, paintOutline);
}

void DrawingProgram::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    layerMan.load_file(a, version);
}

void DrawingProgram::save_file(cereal::PortableBinaryOutputArchive& a) const {
    layerMan.save_file(a);
}

void DrawingProgram::get_used_resources(std::unordered_set<NetworkingObjects::NetObjID>& resourceSet) {
    layerMan.get_used_resources(resourceSet);
}

void DrawingProgram::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(drawData.takingScreenshot)
        layerMan.draw(canvas, drawData);
    else {
        canvas->saveLayer(nullptr, nullptr);
            canvas->clear(SkColor4f{0.0f, 0.0f, 0.0f, 0.0f});
            drawCache.update_and_draw_cached_canvas(canvas, drawData);
            canvas->saveLayer(nullptr, nullptr);
                selection.draw_components(canvas, drawData);
            canvas->restore();
        canvas->restore();

        for(auto& droppedDownFile : droppedDownloadingFiles)
            static_cast<ImageCanvasComponent&>(droppedDownFile.comp->obj->get_comp()).draw_download_progress_bar(canvas, drawData, droppedDownFile.downData->progress);

        for(auto& c : updateableComponents) {
            if(c->obj->get_comp().get_type() == CanvasComponentType::IMAGE) {
                auto& img = static_cast<ImageCanvasComponent&>(c->obj->get_comp());
                float progress = drawData.main->world->rMan.get_resource_retrieval_progress(img.d.imageID);
                img.draw_download_progress_bar(canvas, drawData, progress);
            }
        }

        selection.draw_gui(canvas, drawData);
        drawTool->draw(canvas, drawData);
    }
}

Vector4f* DrawingProgram::get_foreground_color_ptr() {
    return &world.main.toolConfig.globalConf.foregroundColor;
}
