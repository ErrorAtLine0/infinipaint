#include "DrawingProgram.hpp"
#include "DrawingProgramToolBase.hpp"
#include <include/core/SkPaint.h>
#include <include/core/SkVertices.h>
#include "../DrawCamera.hpp"
#include "EllipseDrawTool.hpp"
#include "EyeDropperTool.hpp"
#include "GridModifyTool.hpp"
#include "../InputManager.hpp"
#include "../SharedTypes.hpp"
#include "../Server/CommandList.hpp"
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
#include "LassoSelectTool.hpp"
#include <Helpers/Logger.hpp>
#include <Helpers/Parallel.hpp>
#include <cereal/types/unordered_set.hpp>
#include <include/core/SkPathEffect.h>
#include <include/effects/SkDashPathEffect.h>
#include <chrono>

#include "EraserTool.hpp"

DrawingProgram::DrawingProgram(World& initWorld):
    world(initWorld),
    compCache(*this),
    selection(*this)
{
    drawTool = DrawingProgramToolBase::allocate_tool_type(*this, DrawingProgramToolType::BRUSH);
}

void DrawingProgram::init() {
    if(world.netObjMan.is_server()) {
        components = world.netObjMan.make_obj<NetworkingObjects::NetObjOrderedList<CanvasComponentContainer>>();
        set_component_list_callbacks();
    }
}

void DrawingProgram::set_component_list_callbacks() {
    components->set_insert_callback([&](const CanvasComponentContainer::ObjInfoSharedPtr& c) {
        c->obj->set_owner_obj_info(c);
        c->obj->commit_update(*this); // Run commit update on insert so that world bounds are calculated
        compCache.add_component(c);
    });
    components->set_erase_callback([&](const CanvasComponentContainer::ObjInfoSharedPtr& c) {
        compCache.erase_component(c);
        selection.erase_component(c);
        drawTool->erase_component(c);
    });
    components->set_post_sync_callback([&]() {
        clear_draw_cache();
    });
}

void DrawingProgram::init_client_callbacks() {
}

void DrawingProgram::scale_up(const WorldScalar& scaleUpAmount) {
    //for(auto& c : components->get_data())
    //    c->obj->scale_up(scaleUpAmount);
    //selection.deselect_all();
    //force_rebuild_cache();
    //switch_to_tool(drawTool->get_type() == DrawingProgramToolType::GRIDMODIFY ? DrawingProgramToolType::EDIT : drawTool->get_type(), true);
}

void DrawingProgram::parallel_loop_all_components(std::function<void(const CanvasComponentContainer::ObjInfoSharedPtr&)> func) {
    parallel_loop_container(components->get_data(), func);
}

std::unordered_set<ServerClientID> DrawingProgram::get_used_resources() const {
    std::unordered_set<ServerClientID> toRet;
    //for(auto& c : components->get_data())
    //    c->obj->get_used_resources(toRet);
    return toRet;
}

void DrawingProgram::erase_component_set(const std::unordered_set<CanvasComponentContainer::ObjInfoSharedPtr>& compsToErase) {
    std::unordered_set<NetworkingObjects::NetObjID> idsToErase;
    for(auto& c : compsToErase)
        idsToErase.emplace(c->obj.get_net_id());
    components->erase_unordered_set(components, idsToErase);
}

void DrawingProgram::clear_draw_cache() {
    compCache.clear_own_cached_surfaces();
    selection.clear_own_cached_surfaces();
}

void DrawingProgram::write_components_server(cereal::PortableBinaryOutputArchive& a) {
    components.write_create_message(a);
}

void DrawingProgram::read_components_client(cereal::PortableBinaryInputArchive& a) {
    components = world.netObjMan.read_create_message<NetworkingObjects::NetObjOrderedList<CanvasComponentContainer>>(a, nullptr);
    parallel_loop_all_components([&](const auto& c){
        c->obj->commit_update_dont_invalidate_cache(*this);
    });
    compCache.test_rebuild(components->get_data(), true);
    set_component_list_callbacks();
}

void DrawingProgram::toolbar_gui() {
    Toolbar& t = world.main.toolbar;
    t.gui.push_id("Drawing Program Toolbar GUI");
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0)},
            .padding = CLAY_PADDING_ALL(static_cast<uint16_t>(t.io->theme->padding1 / 2)),
            .childGap = static_cast<uint16_t>(t.io->theme->childGap1 / 2), 
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = convert_vec4<Clay_Color>(t.io->theme->backColor1),
        .cornerRadius = CLAY_CORNER_RADIUS(t.io->theme->windowCorners1)
    }) {
        t.gui.obstructing_window();
        if(t.gui.svg_icon_button_transparent("Brush Toolbar Button", "data/icons/brush.svg", drawTool->get_type() == DrawingProgramToolType::BRUSH)) { switch_to_tool(DrawingProgramToolType::BRUSH); }
        if(t.gui.svg_icon_button_transparent("Eraser Toolbar Button", "data/icons/eraser.svg", drawTool->get_type() == DrawingProgramToolType::ERASER)) { switch_to_tool(DrawingProgramToolType::ERASER); }
        if(t.gui.svg_icon_button_transparent("Text Toolbar Button", "data/icons/text.svg", drawTool->get_type() == DrawingProgramToolType::TEXTBOX)) { switch_to_tool(DrawingProgramToolType::TEXTBOX); }
        if(t.gui.svg_icon_button_transparent("Ellipse Toolbar Button", "data/icons/circle.svg", drawTool->get_type() == DrawingProgramToolType::ELLIPSE)) { switch_to_tool(DrawingProgramToolType::ELLIPSE); }
        if(t.gui.svg_icon_button_transparent("Rect Toolbar Button", "data/icons/rectangle.svg", drawTool->get_type() == DrawingProgramToolType::RECTANGLE)) { switch_to_tool(DrawingProgramToolType::RECTANGLE); }
        if(t.gui.svg_icon_button_transparent("RectSelect Toolbar Button", "data/icons/rectselect.svg", drawTool->get_type() == DrawingProgramToolType::RECTSELECT)) { switch_to_tool(DrawingProgramToolType::RECTSELECT); }
        if(t.gui.svg_icon_button_transparent("LassoSelect Toolbar Button", "data/icons/lassoselect.svg", drawTool->get_type() == DrawingProgramToolType::LASSOSELECT)) { switch_to_tool(DrawingProgramToolType::LASSOSELECT); }
        if(t.gui.svg_icon_button_transparent("Edit Toolbar Button", "data/icons/cursor.svg", drawTool->get_type() == DrawingProgramToolType::EDIT)) { switch_to_tool(DrawingProgramToolType::EDIT); }
        if(t.gui.svg_icon_button_transparent("Eyedropper Toolbar Button", "data/icons/eyedropper.svg", drawTool->get_type() == DrawingProgramToolType::EYEDROPPER)) { switch_to_tool(DrawingProgramToolType::EYEDROPPER); }
        if(t.gui.svg_icon_button_transparent("Zoom Canvas Toolbar Button", "data/icons/zoom.svg", drawTool->get_type() == DrawingProgramToolType::ZOOM)) { switch_to_tool(DrawingProgramToolType::ZOOM); }
        if(t.gui.svg_icon_button_transparent("Pan Canvas Toolbar Button", "data/icons/hand.svg", drawTool->get_type() == DrawingProgramToolType::PAN)) { switch_to_tool(DrawingProgramToolType::PAN); }

        double newRotationAngle = world.drawData.cam.c.rotation, oldRotationAngle = world.drawData.cam.c.rotation;
        t.gui.rotate_wheel("Canvas Rotate Wheel", &newRotationAngle);
        if(newRotationAngle != oldRotationAngle)
            world.drawData.cam.c.rotate_about(world.drawData.cam.c.from_space(world.main.window.size.cast<float>() * 0.5f), newRotationAngle - oldRotationAngle);

        if(t.gui.big_color_button("Foreground Color", &controls.foregroundColor, &controls.foregroundColor == t.colorLeft))
            t.color_selector_left(&controls.foregroundColor == t.colorLeft ? nullptr : &controls.foregroundColor);
        if(t.gui.big_color_button("Background Color", &controls.backgroundColor, &controls.backgroundColor == t.colorLeft))
            t.color_selector_left(&controls.backgroundColor == t.colorLeft ? nullptr : &controls.backgroundColor);
    }
    t.gui.pop_id();
}

// Should only be used in selection allowing tools
bool DrawingProgram::selection_action_menu(Vector2f popupPos) {
    Toolbar& t = world.main.toolbar;
    bool shouldClose = false;
    t.gui.list_popup_menu("Selection popup menu", popupPos, [&]() {
        t.gui.text_label_light("Selection menu");
        if(t.gui.text_button_left_transparent("Paste", "Paste")) {
            selection.deselect_all();
            selection.paste_clipboard(popupPos * t.final_gui_scale());
            shouldClose = true;
        }
        if(selection.is_something_selected()) {
            if(t.gui.text_button_left_transparent("Copy", "Copy")) {
                selection.selection_to_clipboard();
                shouldClose = true;
            }
            if(t.gui.text_button_left_transparent("Cut", "Cut")) {
                selection.selection_to_clipboard();
                selection.delete_all();
                shouldClose = true;
            }
            if(t.gui.text_button_left_transparent("Delete", "Delete")) {
                selection.delete_all();
                shouldClose = true;
            }
        }
    });
    return !shouldClose;
}

bool DrawingProgram::right_click_popup_gui(Vector2f popupPos) {
    Toolbar& t = world.main.toolbar;
    t.gui.push_id("Drawing Program right click GUI");
    bool toRet = drawTool->right_click_popup_gui(popupPos);
    t.gui.pop_id();
    return toRet;
}

void DrawingProgram::tool_options_gui() {
    Toolbar& t = world.main.toolbar;
    float minGUIWidth = drawTool->get_type() == DrawingProgramToolType::SCREENSHOT ? 300 : 200;
    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIT(minGUIWidth), .height = CLAY_SIZING_FIT(0)},
            .padding = CLAY_PADDING_ALL(t.io->theme->padding1),
            .childGap = t.io->theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = convert_vec4<Clay_Color>(t.io->theme->backColor1),
        .cornerRadius = CLAY_CORNER_RADIUS(t.io->theme->windowCorners1)
    }) {
        t.gui.obstructing_window();
        drawTool->gui_toolbox();
    }
}

void DrawingProgram::modify_grid(const NetworkingObjects::NetObjWeakPtr<WorldGrid>& gridToModify) {
    std::unique_ptr<GridModifyTool> newTool(std::make_unique<GridModifyTool>(*this));
    newTool->set_grid(gridToModify);
    switch_to_tool_ptr(std::move(newTool));
}

void DrawingProgram::update() {
    if(world.main.window.lastFrameTime > std::chrono::milliseconds(33)) { // Around 30FPS
        if(!badFrametimeTimePoint)
            badFrametimeTimePoint = std::chrono::steady_clock::now();
        else if(unorderedObjectsExistTimePoint && std::chrono::steady_clock::now() - badFrametimeTimePoint.value() >= std::chrono::seconds(5) && std::chrono::steady_clock::now() - unorderedObjectsExistTimePoint.value() >= std::chrono::seconds(5)) {
            force_rebuild_cache();
            unorderedObjectsExistTimePoint = std::nullopt;
            badFrametimeTimePoint = std::nullopt;
        }
    }
    else
        badFrametimeTimePoint = std::nullopt;

    if(compCache.get_unsorted_component_list().empty()) {
        unorderedObjectsExistTimePoint = std::nullopt;
    }
    else if(!unorderedObjectsExistTimePoint) {
        unorderedObjectsExistTimePoint = std::chrono::steady_clock::now();
    }

    if(world.main.input.pen.isEraser && !temporaryEraser) {
        if(drawTool->get_type() == DrawingProgramToolType::BRUSH)
            switch_to_tool(DrawingProgramToolType::ERASER);
        temporaryEraser = true;
    }
    else if(!world.main.input.pen.isEraser && temporaryEraser) {
        if(drawTool->get_type() == DrawingProgramToolType::ERASER)
            switch_to_tool(DrawingProgramToolType::BRUSH);
        temporaryEraser = false;
    }

    if(world.main.input.key(InputManager::KEY_HOLD_TO_PAN).held && !temporaryPan) {
        toolTypeAfterTempPan = drawTool->get_type();
        switch_to_tool(DrawingProgramToolType::PAN);
        temporaryPan = true;
    }

    if(!world.main.input.key(InputManager::KEY_HOLD_TO_PAN).held && temporaryPan) {
        switch_to_tool(toolTypeAfterTempPan);
        temporaryPan = false;
    }

    controls.cursorHoveringOverCanvas = !world.main.toolbar.io->hoverObstructed;
    controls.leftClick = controls.cursorHoveringOverCanvas && world.main.input.mouse.leftClicks;
    if(controls.leftClick) {
        controls.leftClickHeld = true;
        world.main.toolbar.rightClickPopupLocation = std::nullopt;
    }
    if(controls.leftClickHeld && !world.main.input.mouse.leftDown) {
        controls.leftClick = false;
        controls.leftClickHeld = false;
        controls.leftClickReleased = true;
    }

    controls.middleClick = controls.cursorHoveringOverCanvas && (world.main.input.mouse.middleClicks || world.main.input.pen.buttons[world.main.toolbar.tabletOptions.middleClickButton].pressed);
    bool middleHeld = world.main.input.mouse.middleDown || world.main.input.pen.buttons[world.main.toolbar.tabletOptions.middleClickButton].held;
    if(controls.middleClick) {
        controls.middleClickHeld = true;
        world.main.toolbar.rightClickPopupLocation = std::nullopt;
    }
    if(controls.middleClickHeld && !middleHeld) {
        controls.middleClick = false;
        controls.middleClickHeld = false;
        controls.middleClickReleased = true;
    }

    if(controls.cursorHoveringOverCanvas && (world.main.input.mouse.rightClicks || world.main.input.pen.buttons[world.main.toolbar.tabletOptions.rightClickButton].pressed)) {
        if(world.main.toolbar.rightClickPopupLocation)
            world.main.toolbar.rightClickPopupLocation = std::nullopt;
        else
            world.main.toolbar.rightClickPopupLocation = world.main.input.mouse.pos / world.main.toolbar.final_gui_scale();
    }

    controls.previousMouseWorldPos = controls.currentMouseWorldPos;
    controls.currentMouseWorldPos = world.get_mouse_world_pos();

    if(world.main.input.key(InputManager::KEY_DRAW_TOOL_BRUSH).pressed)
        switch_to_tool(DrawingProgramToolType::BRUSH);
    else if(world.main.input.key(InputManager::KEY_DRAW_TOOL_ERASER).pressed)
        switch_to_tool(DrawingProgramToolType::ERASER);
    else if(world.main.input.key(InputManager::KEY_DRAW_TOOL_RECTSELECT).pressed)
        switch_to_tool(DrawingProgramToolType::RECTSELECT);
    else if(world.main.input.key(InputManager::KEY_DRAW_TOOL_RECTANGLE).pressed)
        switch_to_tool(DrawingProgramToolType::RECTANGLE);
    else if(world.main.input.key(InputManager::KEY_DRAW_TOOL_ELLIPSE).pressed)
        switch_to_tool(DrawingProgramToolType::ELLIPSE);
    else if(world.main.input.key(InputManager::KEY_DRAW_TOOL_TEXTBOX).pressed)
        switch_to_tool(DrawingProgramToolType::TEXTBOX);
    else if(world.main.input.key(InputManager::KEY_DRAW_TOOL_EYEDROPPER).pressed)
        switch_to_tool(DrawingProgramToolType::EYEDROPPER);
    else if(world.main.input.key(InputManager::KEY_DRAW_TOOL_SCREENSHOT).pressed)
        switch_to_tool(DrawingProgramToolType::SCREENSHOT);
    else if(world.main.input.key(InputManager::KEY_DRAW_TOOL_ZOOM).pressed)
        switch_to_tool(DrawingProgramToolType::ZOOM);
    else if(world.main.input.key(InputManager::KEY_DRAW_TOOL_LASSOSELECT).pressed)
        switch_to_tool(DrawingProgramToolType::LASSOSELECT);
    else if(world.main.input.key(InputManager::KEY_DRAW_TOOL_PAN).pressed)
        switch_to_tool(DrawingProgramToolType::PAN);

    selection.update();
    drawTool->tool_update();

    // Switch tools after the tool update, not before, so that we dont have to call erase_component on the toolToSwitchToAfterUpdate as well (components will not be erased in this time period)
    if(toolToSwitchToAfterUpdate) {
        switch_to_tool_ptr(std::move(toolToSwitchToAfterUpdate));
        toolToSwitchToAfterUpdate = nullptr;
    }

    if(controls.leftClickReleased)
        controls.leftClickReleased = false;

    if(drawTool->get_type() == DrawingProgramToolType::ERASER) {
        EraserTool* eraserTool = static_cast<EraserTool*>(drawTool.get());
        compCache.test_rebuild_dont_include_set_dont_include_nodes(components->get_data(), eraserTool->erasedComponents, eraserTool->erasedBVHNodes);
    }
    else if(is_selection_allowing_tool(drawTool->get_type()))
        compCache.test_rebuild_dont_include_set(components->get_data(), selection.get_selected_set());
    else
        compCache.test_rebuild(components->get_data());
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
    world.main.toolbar.rightClickPopupLocation = std::nullopt;
}

void DrawingProgram::switch_to_tool(DrawingProgramToolType newToolType, bool force) {
    if(newToolType != drawTool->get_type() || force) {
        drawTool->switch_tool(newToolType);
        drawTool = DrawingProgramToolBase::allocate_tool_type(*this, newToolType);
        world.main.toolbar.rightClickPopupLocation = std::nullopt;
    }
}

bool DrawingProgram::prevent_undo_or_redo() {
    return drawTool->prevent_undo_or_redo();
}

SkPaint DrawingProgram::select_tool_line_paint() {
    SkScalar intervals[] = {10, 5};
    float timeSinceEpoch = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now().time_since_epoch()).count();
    sk_sp<SkPathEffect> lassoLineDashEffect = SkDashPathEffect::Make({intervals, 2}, -std::fmod(timeSinceEpoch * 50, 15));

    SkPaint selectLinePaint;
    selectLinePaint.setStyle(SkPaint::kStroke_Style);
    selectLinePaint.setStrokeWidth(3);
    selectLinePaint.setColor4f(world.canvasTheme.toolFrontColor);
    selectLinePaint.setPathEffect(lassoLineDashEffect);

    return selectLinePaint;
}

void DrawingProgram::force_rebuild_cache() {
    if(drawTool->get_type() == DrawingProgramToolType::ERASER) {
        EraserTool* eraserTool = static_cast<EraserTool*>(drawTool.get());
        compCache.test_rebuild_dont_include_set_dont_include_nodes(components->get_data(), eraserTool->erasedComponents, eraserTool->erasedBVHNodes, true);
    }
    else if(is_selection_allowing_tool(drawTool->get_type()))
        compCache.test_rebuild_dont_include_set(components->get_data(), selection.get_selected_set(), true);
    else
        compCache.test_rebuild(components->get_data(), true);
}

void DrawingProgram::invalidate_cache_at_component(const CanvasComponentContainer::ObjInfoSharedPtr& objToCheck) {
    if(selection.is_selected(objToCheck))
        selection.invalidate_cache_at_aabb_before_pos(objToCheck->obj->get_world_bounds(), objToCheck->pos);
    else
        compCache.invalidate_cache_at_aabb_before_pos(objToCheck->obj->get_world_bounds(), objToCheck->pos);
}

void DrawingProgram::preupdate_component(const CanvasComponentContainer::ObjInfoSharedPtr& objToCheck) {
    if(selection.is_selected(objToCheck))
        selection.preupdate_component(objToCheck);
    else
        compCache.preupdate_component(objToCheck);
}

float DrawingProgram::drag_point_radius() {
    return 8.0f * world.main.toolbar.final_gui_scale();
}

void DrawingProgram::draw_drag_circle(SkCanvas* canvas, const Vector2f& sPos, const SkColor4f& c, const DrawData& drawData, float radiusMultiplier) {
    float constantRadius = drag_point_radius() * radiusMultiplier;
    float constantThickness = constantRadius * 0.2f;
    canvas->drawCircle(sPos.x(), sPos.y(), constantRadius, SkPaint(c));
    SkPaint paintOutline(SkColor4f{0.95f, 0.95f, 0.95f, 1.0f});
    paintOutline.setStroke(true);
    paintOutline.setStrokeWidth(constantThickness);
    canvas->drawCircle(sPos.x(), sPos.y(), constantRadius, paintOutline);
}

void DrawingProgram::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    //uint64_t compCount;
    //a(compCount);
    //for(uint64_t i = 0; i < compCount; i++) {
    //    DrawComponentType t;
    //    a(t);
    //    auto newComp = DrawComponent::allocate_comp_type(t);
    //    a(newComp->id, newComp->coords);
    //    newComp->load_file(a, version);
    //    components.init_emplace_back(newComp);
    //}
    //parallel_loop_all_components([&](const auto& c){
    //    c->obj->commit_update(*this, false);
    //});
    //compCache.test_rebuild(components.client_list(), true);
}

void DrawingProgram::save_file(cereal::PortableBinaryOutputArchive& a) const {
    //auto& cList = components->get_data();
    //a((uint64_t)cList.size());
    //for(size_t i = 0; i < cList.size(); i++) {
    //    a(cList[i]->obj->get_type(), cList[i]->obj->id, cList[i]->obj->coords);
    //    cList[i]->obj->save_file(a);
    //}
}

void DrawingProgram::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(drawData.dontUseDrawProgCache) {
        for(auto& c : components->get_data())
            c->obj->draw(canvas, drawData);
    }
    else {
        canvas->saveLayer(nullptr, nullptr);
            canvas->clear(SkColor4f{0.0f, 0.0f, 0.0f, 0.0f});
            compCache.refresh_all_draw_cache(drawData);
            compCache.draw_components_to_canvas(canvas, drawData, {});
            canvas->saveLayer(nullptr, nullptr);
                selection.draw_components(canvas, drawData);
            canvas->restore();
        canvas->restore();
        selection.draw_gui(canvas, drawData);
    }

    drawTool->draw(canvas, drawData);
}

Vector4f* DrawingProgram::get_foreground_color_ptr() {
    return &controls.foregroundColor;
}
