#include "DrawingProgram.hpp"
#include "DrawingProgramToolBase.hpp"
#include <include/core/SkPaint.h>
#include <include/core/SkVertices.h>
#include "../DrawCamera.hpp"
#include "../DrawComponents/DrawComponent.hpp"
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
#include "../DrawComponents/DrawImage.hpp"
#include <include/core/SkImage.h>
#include <include/core/SkSurface.h>
#include "../World.hpp"
#include "../MainProgram.hpp"
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
    components([&](){ return world.get_new_id(); }),
    selection(*this)
{
    drawTool = DrawingProgramToolBase::allocate_tool_type(*this, DrawingProgramToolType::BRUSH);

    components.clientInsertCallback = [&](const CollabListType::ObjectInfoPtr& c) {
        if(addToCompCacheOnInsert)
            compCache.add_component(c);
        DrawComponentType t = c->obj->get_type();
        if(t == DRAWCOMPONENT_IMAGE || t == DRAWCOMPONENT_TEXTBOX)
            updateableComponents.emplace(c->obj);
    };
    components.clientEraseCallback = [&](const CollabListType::ObjectInfoPtr& c) {
        compCache.erase_component(c);
        selection.erase_component(c);
        delayedUpdateComponents.erase(c->obj);
        updateableComponents.erase(c->obj);
    };
    components.clientInsertOrderedVectorCallback = [&](const std::vector<CollabListType::ObjectInfoPtr>& comps) {
        for(auto& c : comps)
            components.clientInsertCallback(c);
    };
    components.clientServerFirstPosShiftCallback = [&](uint64_t firstShiftPos) {
        clear_draw_cache();
    };
}

void DrawingProgram::init_client_callbacks() {
    world.con.client_add_recv_callback(CLIENT_UPDATE_COMPONENT, [&](cereal::PortableBinaryInputArchive& message) {
        ServerClientID id;
        message(id);
        auto objPtr = components.get_item_by_id(id);
        if(!objPtr)
            return;
        std::shared_ptr<DrawComponent>& comp = objPtr->obj;
        if((std::chrono::steady_clock::now() - comp->lastUpdateTime) > CLIENT_DRAWCOMP_DELAY_TIMER_DURATION) {
            delayedUpdateComponents.erase(comp);
            message(*comp);
            comp->commit_update(*this);
        }
        else {
            auto delayedUpdatePtr = DrawComponent::allocate_comp_type(comp->get_type());
            message(*delayedUpdatePtr);
            delayedUpdateComponents[comp] = delayedUpdatePtr;
        }
    });
    world.con.client_add_recv_callback(CLIENT_TRANSFORM_MANY_COMPONENTS, [&](cereal::PortableBinaryInputArchive& message) {
        std::vector<std::pair<ServerClientID, CoordSpaceHelper>> transforms;
        message(transforms);
        if(transforms.size() >= DrawingProgramCache::MINIMUM_COMPONENTS_TO_START_REBUILD) {
            std::atomic<size_t> objsActuallyTransformed = 0;
            parallel_loop_container(transforms, [&](auto& p) {
                auto& [id, coords] = p;
                auto objPtr = components.get_item_by_id(id);
                if(!objPtr || selection.is_selected(objPtr)) // Whatever transformation we're doing right now will overwrite this transformation, so we can ignore this message
                    return;
                std::shared_ptr<DrawComponent>& comp = objPtr->obj;
                if(comp->coords == coords)
                    return;
                comp->coords = coords;
                objsActuallyTransformed++;
                comp->commit_transform(*this, false);
            });
            if(objsActuallyTransformed != 0)
                force_rebuild_cache();
        }
        else {
            for(auto& [id, coords] : transforms) {
                auto objPtr = components.get_item_by_id(id);
                if(!objPtr || selection.is_selected(objPtr)) // Whatever transformation we're doing right now will overwrite this transformation, so we can ignore this message
                    continue;
                std::shared_ptr<DrawComponent>& comp = objPtr->obj;
                if(comp->coords == coords)
                    return;
                comp->coords = coords;
                comp->commit_transform(*this);
            }
        }
    });
    world.con.client_add_recv_callback(CLIENT_PLACE_SINGLE_COMPONENT, [&](cereal::PortableBinaryInputArchive& message) {
        uint64_t insertPosition;
        DrawComponentType type;
        message(insertPosition, type);
        auto newObj = DrawComponent::allocate_comp_type(type);
        message(newObj->id, newObj->coords, *newObj);
        bool didntExistPreviously = components.server_insert(insertPosition, newObj);
        if(didntExistPreviously)
            newObj->commit_update(*this);
    });
    world.con.client_add_recv_callback(CLIENT_PLACE_MANY_COMPONENTS, [&](cereal::PortableBinaryInputArchive& message) {
        std::vector<CollabListType::ObjectInfoPtr> sortedPlacedComponents;

        SendOrderedComponentVectorOp a{&sortedPlacedComponents};
        message(a);

        std::vector<bool> didntExistPreviously = components.server_insert_ordered_vector(sortedPlacedComponents);
        bool isSingleThread = sortedPlacedComponents.size() < DrawingProgramCache::MINIMUM_COMPONENTS_TO_START_REBUILD;
        parallel_loop_container(sortedPlacedComponents, [&](const auto& c){
            c->obj->commit_update(*this, isSingleThread);
        }, isSingleThread);
        if(!isSingleThread)
            force_rebuild_cache();
    });
    world.con.client_add_recv_callback(CLIENT_ERASE_SINGLE_COMPONENT, [&](cereal::PortableBinaryInputArchive& message) {
        ServerClientID idToErase;
        message(idToErase);
        components.server_erase(idToErase);
    });
    world.con.client_add_recv_callback(CLIENT_ERASE_MANY_COMPONENTS, [&](cereal::PortableBinaryInputArchive& message) {
        std::unordered_set<ServerClientID> idsToErase;
        message(idsToErase);
        components.server_erase_set(idsToErase);
    });
}

void DrawingProgram::check_delayed_update_timers() {
    std::erase_if(delayedUpdateComponents, [&](auto& p) {
        auto& [comp, delayedUpdatePtr] = p;
        return comp->check_timers(*this, delayedUpdatePtr);
    });
}

void DrawingProgram::check_updateable_components() {
    for(auto& comp : updateableComponents)
        comp->update(*this);
}

void DrawingProgram::parallel_loop_all_components(std::function<void(const std::shared_ptr<CollabList<std::shared_ptr<DrawComponent>, ServerClientID>::ObjectInfo>&)> func) {
    parallel_loop_container(components.client_list(), func);
}

std::unordered_set<ServerClientID> DrawingProgram::get_used_resources() {
    std::unordered_set<ServerClientID> toRet;
    for(auto& c : components.client_list())
        c->obj->get_used_resources(toRet);
    return toRet;
}

void DrawingProgram::clear_draw_cache() {
    compCache.clear_own_cached_surfaces();
    selection.clear_own_cached_surfaces();
}

void DrawingProgram::toolbar_gui() {
    Toolbar& t = world.main.toolbar;
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIT(0), .height = CLAY_SIZING_FIT(0)},
            .padding = CLAY_PADDING_ALL(t.io->theme->padding1),
            .childGap = t.io->theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = convert_vec4<Clay_Color>(t.io->theme->backColor1),
        .cornerRadius = CLAY_CORNER_RADIUS(t.io->theme->windowCorners1),
        .border = {.color = convert_vec4<Clay_Color>(t.io->theme->backColor2), .width = CLAY_BORDER_OUTSIDE(t.io->theme->windowBorders1)}
    }) {
        t.gui.obstructing_window();
        if(t.gui.svg_icon_button("Brush Toolbar Button", "data/icons/brush.svg", drawTool->get_type() == DrawingProgramToolType::BRUSH)) { switch_to_tool(DrawingProgramToolType::BRUSH); }
        if(t.gui.svg_icon_button("Eraser Toolbar Button", "data/icons/eraser.svg", drawTool->get_type() == DrawingProgramToolType::ERASER)) { switch_to_tool(DrawingProgramToolType::ERASER); }
        if(t.gui.svg_icon_button("Text Toolbar Button", "data/icons/text.svg", drawTool->get_type() == DrawingProgramToolType::TEXTBOX)) { switch_to_tool(DrawingProgramToolType::TEXTBOX); }
        if(t.gui.svg_icon_button("Ellipse Toolbar Button", "data/icons/circle.svg", drawTool->get_type() == DrawingProgramToolType::ELLIPSE)) { switch_to_tool(DrawingProgramToolType::ELLIPSE); }
        if(t.gui.svg_icon_button("Rect Toolbar Button", "data/icons/rectangle.svg", drawTool->get_type() == DrawingProgramToolType::RECTANGLE)) { switch_to_tool(DrawingProgramToolType::RECTANGLE); }
        if(t.gui.svg_icon_button("RectSelect Toolbar Button", "data/icons/rectselect.svg", drawTool->get_type() == DrawingProgramToolType::RECTSELECT)) { switch_to_tool(DrawingProgramToolType::RECTSELECT); }
        if(t.gui.svg_icon_button("LassoSelect Toolbar Button", "data/icons/lassoselect.svg", drawTool->get_type() == DrawingProgramToolType::LASSOSELECT)) { switch_to_tool(DrawingProgramToolType::LASSOSELECT); }
        if(t.gui.svg_icon_button("Edit Toolbar Button", "data/icons/cursor.svg", drawTool->get_type() == DrawingProgramToolType::EDIT)) { switch_to_tool(DrawingProgramToolType::EDIT); }
        if(t.gui.svg_icon_button("Eyedropper Toolbar Button", "data/icons/eyedropper.svg", drawTool->get_type() == DrawingProgramToolType::EYEDROPPER)) { switch_to_tool(DrawingProgramToolType::EYEDROPPER); }
        if(t.gui.svg_icon_button("Zoom Canvas Toolbar Button", "data/icons/zoom.svg", drawTool->get_type() == DrawingProgramToolType::ZOOM)) { switch_to_tool(DrawingProgramToolType::ZOOM); }
        if(t.gui.svg_icon_button("Pan Canvas Toolbar Button", "data/icons/hand.svg", drawTool->get_type() == DrawingProgramToolType::PAN)) { switch_to_tool(DrawingProgramToolType::PAN); }
        CLAY({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(40), .height = CLAY_SIZING_FIXED(40)}}}) {
            if(t.gui.color_button("Foreground Color", &controls.foregroundColor, &controls.foregroundColor == t.colorLeft)) {
                t.color_selector_left(&controls.foregroundColor == t.colorLeft ? nullptr : &controls.foregroundColor);
            }
        }
        CLAY({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(40), .height = CLAY_SIZING_FIXED(40)}}}) {
            if(t.gui.color_button("Background Color", &controls.backgroundColor, &controls.backgroundColor == t.colorLeft))
                t.color_selector_left(&controls.backgroundColor == t.colorLeft ? nullptr : &controls.backgroundColor);
        }
    }
}

void DrawingProgram::tool_options_gui() {
    Toolbar& t = world.main.toolbar;
    float minGUIWidth = drawTool->get_type() == DrawingProgramToolType::SCREENSHOT ? 300 : 200;
    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIT(minGUIWidth), .height = CLAY_SIZING_FIT(0)},
            .padding = CLAY_PADDING_ALL(t.io->theme->padding1),
            .childGap = t.io->theme->childGap1,
            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
            .layoutDirection = CLAY_TOP_TO_BOTTOM
        },
        .backgroundColor = convert_vec4<Clay_Color>(t.io->theme->backColor1),
        .cornerRadius = CLAY_CORNER_RADIUS(t.io->theme->windowCorners1),
        .border = {.color = convert_vec4<Clay_Color>(t.io->theme->backColor2), .width = CLAY_BORDER_OUTSIDE(t.io->theme->windowBorders1)}
    }) {
        t.gui.obstructing_window();
        drawTool->gui_toolbox();
    }
}

void DrawingProgram::modify_grid(ServerClientID gridToModifyID) {
    std::unique_ptr<GridModifyTool> newTool(std::make_unique<GridModifyTool>(*this));
    newTool->set_grid_id(gridToModifyID);
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

    if(drawTool->get_type() == DrawingProgramToolType::BRUSH && world.main.input.pen.isEraser && !temporaryEraser) {
        switch_to_tool(DrawingProgramToolType::ERASER);
        temporaryEraser = true;
    }
    else if(drawTool->get_type() == DrawingProgramToolType::ERASER && !world.main.input.pen.isEraser && temporaryEraser) {
        switch_to_tool(DrawingProgramToolType::BRUSH);
        temporaryEraser = false;
    }

    controls.cursorHoveringOverCanvas = !world.main.toolbar.io->hoverObstructed;
    controls.leftClick = controls.cursorHoveringOverCanvas && world.main.input.mouse.leftClicks;
    if(controls.leftClick) {
        controls.leftClickHeld = true;
        world.main.toolbar.paintPopupLocation = std::nullopt;
    }
    if(controls.leftClickHeld && !world.main.input.mouse.leftDown) {
        controls.leftClick = false;
        controls.leftClickHeld = false;
        controls.leftClickReleased = true;
    }

    controls.middleClick = controls.cursorHoveringOverCanvas && (world.main.input.mouse.middleClicks || world.main.input.pen.buttons[world.main.toolbar.tabletOptions.middleClickButton].pressed);
    if(controls.middleClick) {
        controls.middleClickHeld = true;
        world.main.toolbar.paintPopupLocation = std::nullopt;
    }
    if(controls.middleClickHeld && !world.main.input.mouse.middleDown) {
        controls.middleClick = false;
        controls.middleClickHeld = false;
        controls.middleClickReleased = true;
    }

    if(controls.cursorHoveringOverCanvas && (world.main.input.mouse.rightClicks || world.main.input.pen.buttons[world.main.toolbar.tabletOptions.rightClickButton].pressed)) {
        if(world.main.toolbar.paintPopupLocation)
            world.main.toolbar.paintPopupLocation = std::nullopt;
        else
            world.main.toolbar.paintPopupLocation = world.main.input.mouse.pos / world.main.toolbar.final_gui_scale();
    }

    controls.previousMouseWorldPos = controls.currentMouseWorldPos;
    controls.currentMouseWorldPos = world.get_mouse_world_pos();

    if(addFileInNextFrame) {
        add_file_to_canvas_by_path_execute(addFileInfo.first, addFileInfo.second);
        addFileInNextFrame = false;
    }

    drag_drop_update();

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
    else if(world.main.input.key(InputManager::KEY_DRAW_TOOL_EDIT).pressed)
        switch_to_tool(DrawingProgramToolType::EDIT);

    selection.update();
    drawTool->tool_update();

    if(controls.leftClickReleased)
        controls.leftClickReleased = false;

    check_delayed_update_timers();
    check_updateable_components();

    if(drawTool->get_type() == DrawingProgramToolType::ERASER) {
        EraserTool* eraserTool = static_cast<EraserTool*>(drawTool.get());
        compCache.test_rebuild_dont_include_set_dont_include_nodes(components.client_list(), eraserTool->erasedComponents, eraserTool->erasedBVHNodes);
    }
    else if(drawTool->get_type() == DrawingProgramToolType::RECTSELECT)
        compCache.test_rebuild_dont_include_set(components.client_list(), selection.get_selected_set());
    else
        compCache.test_rebuild(components.client_list());
}

void DrawingProgram::switch_to_tool_ptr(std::unique_ptr<DrawingProgramToolBase> newTool) {
    drawTool->switch_tool(newTool->get_type());
    drawTool = std::move(newTool);
}

void DrawingProgram::switch_to_tool(DrawingProgramToolType newToolType) {
    if(newToolType != drawTool->get_type()) {
        drawTool->switch_tool(newToolType);
        drawTool = DrawingProgramToolBase::allocate_tool_type(*this, newToolType);
    }
}

bool DrawingProgram::prevent_undo_or_redo() {
    return drawTool->prevent_undo_or_redo();
}

SkPaint DrawingProgram::select_tool_line_paint() {
    SkScalar intervals[] = {10, 5};
    float timeSinceEpoch = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now().time_since_epoch()).count();
    sk_sp<SkPathEffect> lassoLineDashEffect = SkDashPathEffect::Make(intervals, 2, -std::fmod(timeSinceEpoch * 50, 15));

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
        compCache.test_rebuild_dont_include_set_dont_include_nodes(components.client_list(), eraserTool->erasedComponents, eraserTool->erasedBVHNodes, true);
    }
    else if(drawTool->get_type() == DrawingProgramToolType::RECTSELECT || drawTool->get_type() == DrawingProgramToolType::LASSOSELECT)
        compCache.test_rebuild_dont_include_set(components.client_list(), selection.get_selected_set(), true);
    else
        compCache.test_rebuild(components.client_list(), true);
}

void DrawingProgram::drag_drop_update() {
    if(controls.cursorHoveringOverCanvas) {
        for(auto& droppedItem : world.main.input.droppedItems) {
            if(droppedItem.isFile && std::filesystem::is_regular_file(droppedItem.droppedData)) {
#ifdef __EMSCRIPTEN_
                add_file_to_canvas_by_path(droppedItem.droppedData, world.main.input.mouse.pos, true);
#else
                add_file_to_canvas_by_path(droppedItem.droppedData, droppedItem.pos, true);
#endif
            }
        }
    }
}

void DrawingProgram::add_file_to_canvas_by_path(const std::string& filePath, Vector2f dropPos, bool addInSameThread) {
    if(addInSameThread)
        add_file_to_canvas_by_path_execute(filePath, dropPos);
    else if(!addFileInNextFrame) {
        addFileInfo = {filePath, dropPos};
        addFileInNextFrame = true;
    }
}

void DrawingProgram::add_file_to_canvas_by_path_execute(const std::string& filePath, Vector2f dropPos) {
    ServerClientID imageID = world.rMan.add_resource_file(filePath);
    std::shared_ptr<ResourceDisplay> display = world.rMan.get_display_data(imageID);
    Vector2f imTrueDim = display->get_dimensions();
    auto img(std::make_shared<DrawImage>());
    img->coords = world.drawData.cam.c;
    float imWidth = imTrueDim.x() / (imTrueDim.x() + imTrueDim.y());
    float imHeight = imTrueDim.y() / (imTrueDim.x() + imTrueDim.y());
    Vector2f imDim = Vector2f{world.main.window.size.x() * imWidth, world.main.window.size.x() * imHeight} * display->get_dimension_scale();
    img->d.p1 = dropPos - imDim;
    img->d.p2 = dropPos + imDim;
    img->d.imageID = imageID;
    img->commit_update(*this);
    uint64_t placement = components.client_list().size();
    auto objAdd = components.client_insert(placement, img);
    img->client_send_place(*this);
    add_undo_place_component(objAdd);
}

void DrawingProgram::add_file_to_canvas_by_data(const std::string& fileName, std::string_view fileBuffer, Vector2f dropPos) {
    ResourceData newResource;
    newResource.data = std::make_shared<std::string>(fileBuffer);
    newResource.name = fileName;
    ServerClientID imageID = world.rMan.add_resource(newResource);

    std::shared_ptr<ResourceDisplay> display = world.rMan.get_display_data(imageID);
    Vector2f imTrueDim = display->get_dimensions();
    auto img(std::make_shared<DrawImage>());
    img->coords = world.drawData.cam.c;
    float imWidth = imTrueDim.x() / (imTrueDim.x() + imTrueDim.y());
    float imHeight = imTrueDim.y() / (imTrueDim.x() + imTrueDim.y());
    Vector2f imDim = Vector2f{world.main.window.size.x() * imWidth, world.main.window.size.x() * imHeight} * display->get_dimension_scale();
    img->d.p1 = dropPos - imDim;
    img->d.p2 = dropPos + imDim;
    img->d.imageID = imageID;
    img->commit_update(*this);
    uint64_t placement = components.client_list().size();
    auto objAdd = components.client_insert(placement, img);
    img->client_send_place(*this);
    add_undo_place_component(objAdd);
}

void DrawingProgram::initialize_draw_data(cereal::PortableBinaryInputArchive& a) {
    uint64_t compCount;
    a(compCount);
    for(uint64_t i = 0; i < compCount; i++) {
        DrawComponentType t;
        a(t);
        auto newComp = DrawComponent::allocate_comp_type(t);
        a(newComp->id, newComp->coords, *newComp);
        components.init_emplace_back(newComp);
    }
    parallel_loop_all_components([&](const auto& c){
        c->obj->commit_update(*this, false);
    });
    compCache.test_rebuild(components.client_list(), true);
}

void DrawingProgram::client_erase_set(std::unordered_set<CollabListType::ObjectInfoPtr> erasedComponents) {
    std::unordered_set<ServerClientID> idsToErase;
    for(auto& c : erasedComponents)
        idsToErase.emplace(c->obj->id);
    DrawComponent::client_send_erase_set(*this, idsToErase);
    components.client_erase_set(erasedComponents);
    add_undo_erase_components(erasedComponents);
}

void DrawingProgram::add_undo_place_component(const CollabListType::ObjectInfoPtr& objToUndo) {
    world.undo.push(UndoManager::UndoRedoPair{
        [&, objToUndo]() {
            objToUndo->obj->client_send_erase(*this);
            components.client_erase(objToUndo);
            return true;
        },
        [&, objToUndo]() {
            if(objToUndo->obj->collabListInfo.lock())
                return false;
            components.client_insert(objToUndo, false);
            objToUndo->obj->client_send_place(*this);
            return true;
        }
    });
}

void DrawingProgram::invalidate_cache_at_component(const CollabListType::ObjectInfoPtr& objToCheck) {
    if(selection.is_selected(objToCheck))
        selection.invalidate_cache_at_optional_aabb_before_pos(objToCheck->obj->worldAABB, objToCheck->pos);
    else
        compCache.invalidate_cache_at_optional_aabb_before_pos(objToCheck->obj->worldAABB, objToCheck->pos);
}

void DrawingProgram::preupdate_component(const CollabListType::ObjectInfoPtr& objToCheck) {
    if(selection.is_selected(objToCheck))
        selection.preupdate_component(objToCheck);
    else
        compCache.preupdate_component(objToCheck);
}

void DrawingProgram::add_undo_place_components(const std::unordered_set<CollabListType::ObjectInfoPtr>& objSetToUndo) {
    world.undo.push(UndoManager::UndoRedoPair{
        [&, objSetToUndo = objSetToUndo]() {
            for(auto& comp : objSetToUndo)
                if(!comp->obj->collabListInfo.lock())
                    return false;

            std::unordered_set<ServerClientID> idsToErase;
            for(auto& c : objSetToUndo)
                idsToErase.emplace(c->obj->id);

            DrawComponent::client_send_erase_set(*this, idsToErase);
            components.client_erase_set(objSetToUndo);

            return true;
        },
        [&, objSetToUndo = objSetToUndo]() {
            std::vector<CollabListType::ObjectInfoPtr> sortedObjects(objSetToUndo.begin(), objSetToUndo.end());
            std::sort(sortedObjects.begin(), sortedObjects.end(), [](auto& a, auto& b) {
                return a->pos < b->pos;
            });

            for(auto& comp : sortedObjects)
                if(comp->obj->collabListInfo.lock())
                    return false;

            components.client_insert_ordered_vector(sortedObjects, false);
            DrawComponent::client_send_place_many(*this, sortedObjects);

            return true;
        }
    });
}

void DrawingProgram::add_undo_erase_components(const std::unordered_set<CollabListType::ObjectInfoPtr>& objSetToUndo) {
    world.undo.push(UndoManager::UndoRedoPair{
        [&, objSetToUndo = objSetToUndo]() {
            std::vector<CollabListType::ObjectInfoPtr> sortedObjects(objSetToUndo.begin(), objSetToUndo.end());
            std::sort(sortedObjects.begin(), sortedObjects.end(), [](auto& a, auto& b) {
                return a->pos < b->pos;
            });

            for(auto& comp : sortedObjects)
                if(comp->obj->collabListInfo.lock())
                    return false;

            components.client_insert_ordered_vector(sortedObjects, false);
            DrawComponent::client_send_place_many(*this, sortedObjects);

            return true;
        },
        [&, objSetToUndo = objSetToUndo]() {
            for(auto& comp : objSetToUndo)
                if(!comp->obj->collabListInfo.lock())
                    return false;

            std::unordered_set<ServerClientID> idsToErase;
            for(auto& c : objSetToUndo)
                idsToErase.emplace(c->obj->id);

            DrawComponent::client_send_erase_set(*this, idsToErase);
            components.client_erase_set(objSetToUndo);

            return true;
        }
    });
}

ClientPortionID DrawingProgram::get_max_id(ServerPortionID serverID) {
    ClientPortionID maxClientID = 0;
    auto& cList = components.client_list();
    for(auto& comp : cList) {
        if(comp->obj->id.first == serverID)
            maxClientID = std::max(maxClientID, comp->obj->id.second);
    }
    return maxClientID;
}

void DrawingProgram::draw_drag_circle(SkCanvas* canvas, const Vector2f& sPos, const SkColor4f& c, const DrawData& drawData, float radiusMultiplier) {
    float constantRadius = DRAG_POINT_RADIUS * radiusMultiplier;
    float constantThickness = constantRadius * 0.2f;
    canvas->drawCircle(sPos.x(), sPos.y(), constantRadius, SkPaint(c));
    SkPaint paintOutline(SkColor4f{0.95f, 0.95f, 0.95f, 1.0f});
    paintOutline.setStroke(true);
    paintOutline.setStrokeWidth(constantThickness);
    canvas->drawCircle(sPos.x(), sPos.y(), constantRadius, paintOutline);
}

void DrawingProgram::write_to_file(cereal::PortableBinaryOutputArchive& a) {
    auto& cList = components.client_list();
    a((uint64_t)cList.size());
    for(size_t i = 0; i < cList.size(); i++) {
        a(cList[i]->obj->get_type(), cList[i]->obj->id, cList[i]->obj->coords, *cList[i]->obj);
    }
}

void DrawingProgram::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(drawData.dontUseDrawProgCache) {
        parallel_loop_all_components([&](auto& c) {
            c->obj->calculate_draw_transform(drawData);
        });
        for(auto& c : components.client_list()) {
            c->obj->draw(canvas, drawData);
            c->obj->drawSetupData.shouldDraw = false;
        }
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
