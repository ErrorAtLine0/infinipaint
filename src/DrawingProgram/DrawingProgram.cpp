#include "DrawingProgram.hpp"
#include "Helpers/Hashes.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/SCollision.hpp"
#include <include/core/SkPaint.h>
#include <include/core/SkVertices.h>
#include "../DrawCamera.hpp"
#include "../DrawComponents/DrawComponent.hpp"
#include "EllipseDrawTool.hpp"
#include "InkDropperTool.hpp"
#include "../InputManager.hpp"
#include "../SharedTypes.hpp"
#include "../Server/CommandList.hpp"
#include <optional>
#include <ranges>
#include <Helpers/ConvertVec.hpp>
#include <cereal/types/vector.hpp>
#include <iostream>
#include "../DrawComponents/DrawImage.hpp"
#include <include/core/SkImage.h>
#include <include/core/SkSurface.h>
#include "../World.hpp"
#include "../MainProgram.hpp"
#include <Helpers/Logger.hpp>
#include <Helpers/Parallel.hpp>
#include <cereal/types/unordered_set.hpp>

DrawingProgram::DrawingProgram(World& initWorld):
    world(initWorld),
    compCache(*this),
    components([&](){ return world.get_new_id(); }),
    brushTool(*this),
    eraserTool(*this),
    rectSelectTool(*this),
    rectDrawTool(*this),
    ellipseDrawTool(*this),
    textBoxTool(*this),
    inkDropperTool(*this),
    screenshotTool(*this),
    editTool(*this),
    imageTool(*this),
    selection(*this)
{
    components.updateCallback = [&]() {
        if(&world == world.main.world.get())
            world.main.drawProgCache.refresh = true;
    };
    components.clientInsertCallback = [&](const CollabListType::ObjectInfoPtr& c) {
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
    components.clientServerLastPosShiftCallback = [&](uint64_t lastShiftPos) {
        compCache.invalidate_cache_before_pos(lastShiftPos);
    };
}

void DrawingProgram::init_client_callbacks() {
    world.con.client_add_recv_callback(CLIENT_UPDATE_COMPONENT, [&](cereal::PortableBinaryInputArchive& message) {
        bool isTemp;
        ServerClientID id;
        message(isTemp, id);
        auto objPtr = components.get_item_by_id(id);
        if(!objPtr)
            return;
        std::shared_ptr<DrawComponent>& comp = objPtr->obj;
        float dur = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - comp->lastUpdateTime).count();
        if(dur > CLIENT_DRAWCOMP_DELAY_TIMER_DURATION){
            comp->delayedUpdatePtr = nullptr;
            message(*comp);
            if(isTemp)
                comp->temp_update(*this);
            else
                comp->final_update(*this);
        }
        else {
            comp->delayedUpdatePtr = DrawComponent::allocate_comp_type(comp->get_type());
            message(*comp->delayedUpdatePtr);
            delayedUpdateComponents.emplace(comp);
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
                comp->finalize_update(*this, false);
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
                comp->finalize_update(*this); // No need to update draw data during transformation
            }
        }
    });
    world.con.client_add_recv_callback(CLIENT_PLACE_SINGLE_COMPONENT, [&](cereal::PortableBinaryInputArchive& message) {
        uint64_t insertPosition;
        DrawComponentType type;
        message(insertPosition, type);
        ServerClientID newObjectID;
        auto newObj = DrawComponent::allocate_comp_type(type);
        message(newObjectID, newObj->coords, *newObj);
        bool didntExistPreviously = components.server_insert(insertPosition, newObjectID, newObj);
        if(didntExistPreviously)
            newObj->final_update(*this);
    });
    world.con.client_add_recv_callback(CLIENT_PLACE_MANY_COMPONENTS, [&](cereal::PortableBinaryInputArchive& message) {
        std::vector<CollabListType::ObjectInfoPtr> sortedPlacedComponents;

        SendOrderedComponentVectorOp a{&sortedPlacedComponents};
        message(a);

        std::vector<bool> didntExistPreviously = components.server_insert_ordered_vector(sortedPlacedComponents);
        bool isSingleThread = sortedPlacedComponents.size() < DrawingProgramCache::MINIMUM_COMPONENTS_TO_START_REBUILD;
        parallel_loop_container(sortedPlacedComponents, [&](const auto& c){
            c->obj->final_update(*this, isSingleThread);
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
    std::erase_if(delayedUpdateComponents, [&](auto& comp) {
        comp->check_timers(*this);
        return !comp->delayedUpdatePtr;
    });
}

void DrawingProgram::check_updateable_components() {
    for(auto& comp : updateableComponents)
        comp->update(*this);
}

void DrawingProgram::free_collider_memory() {
    for(auto& c : components.client_list())
        c->obj->free_collider();
    colliderAllocated = false;
}

void DrawingProgram::allocate_collider_memory() {
    for(auto& c : components.client_list())
        c->obj->allocate_collider();
    colliderAllocated = true;
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
        if(t.gui.svg_icon_button("Brush Toolbar Button", "data/icons/brush.svg", controls.selectedTool == TOOL_BRUSH)) { controls.selectedTool = TOOL_BRUSH; }
        if(t.gui.svg_icon_button("Eraser Toolbar Button", "data/icons/eraser.svg", controls.selectedTool == TOOL_ERASER)) { controls.selectedTool = TOOL_ERASER; }
        if(t.gui.svg_icon_button("Text Toolbar Button", "data/icons/text.svg", controls.selectedTool == TOOL_TEXTBOX)) { controls.selectedTool = TOOL_TEXTBOX; }
        if(t.gui.svg_icon_button("Ellipse Toolbar Button", "data/icons/circle.svg", controls.selectedTool == TOOL_ELLIPSE)) { controls.selectedTool = TOOL_ELLIPSE; }
        if(t.gui.svg_icon_button("Rect Toolbar Button", "data/icons/rectangle.svg", controls.selectedTool == TOOL_RECTANGLE)) { controls.selectedTool = TOOL_RECTANGLE; }
        if(t.gui.svg_icon_button("RectSelect Toolbar Button", "data/icons/rectselect.svg", controls.selectedTool == TOOL_RECTSELECT)) { controls.selectedTool = TOOL_RECTSELECT; }
        if(t.gui.svg_icon_button("Edit Toolbar Button", "data/icons/cursor.svg", controls.selectedTool == TOOL_EDIT)) { controls.selectedTool = TOOL_EDIT; }
        if(t.gui.svg_icon_button("Inkdropper Toolbar Button", "data/icons/eyedropper.svg", controls.selectedTool == TOOL_INKDROPPER)) { controls.selectedTool = TOOL_INKDROPPER; }
        if(t.gui.svg_icon_button("Screenshot Toolbar Button", "data/icons/camera.svg", controls.selectedTool == TOOL_SCREENSHOT)) { controls.selectedTool = TOOL_SCREENSHOT; }
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
    float minGUIWidth = controls.selectedTool == TOOL_SCREENSHOT ? 300 : 200;
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
        switch(controls.selectedTool) {
            case TOOL_BRUSH:
                brushTool.gui_toolbox();
                break;
            case TOOL_ERASER:
                eraserTool.gui_toolbox();
                break;
            case TOOL_RECTSELECT:
                rectSelectTool.gui_toolbox();
                break;
            case TOOL_RECTANGLE:
                rectDrawTool.gui_toolbox();
                break;
            case TOOL_ELLIPSE:
                ellipseDrawTool.gui_toolbox();
                break;
            case TOOL_TEXTBOX:
                textBoxTool.gui_toolbox();
                break;
            case TOOL_INKDROPPER:
                inkDropperTool.gui_toolbox();
                break;
            case TOOL_SCREENSHOT:
                screenshotTool.gui_toolbox();
                break;
            case TOOL_EDIT:
                editTool.gui_toolbox();
                break;
            default:
                break;
        }
    }
}

void DrawingProgram::update() {
    if(controls.selectedTool == TOOL_BRUSH && world.main.input.pen.isEraser && !temporaryEraser) {
        controls.selectedTool = TOOL_ERASER;
        temporaryEraser = true;
    }
    else if(controls.selectedTool == TOOL_ERASER && !world.main.input.pen.isEraser && temporaryEraser) {
        controls.selectedTool = TOOL_BRUSH;
        temporaryEraser = false;
    }

    if(controls.selectedTool != controls.previousSelected) {
        controls.previousSelected = controls.selectedTool;
        reset_tools();
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

    if(controls.cursorHoveringOverCanvas && (world.main.input.mouse.rightClicks || world.main.input.pen.buttons[world.main.toolbar.tabletOptions.rightClickButton].pressed)) {
        if(world.main.toolbar.paintPopupLocation)
            world.main.toolbar.paintPopupLocation = std::nullopt;
        else
            world.main.toolbar.paintPopupLocation = world.main.input.mouse.pos / world.main.window.scale;
    }

    controls.previousMouseWorldPos = controls.currentMouseWorldPos;
    controls.currentMouseWorldPos = world.get_mouse_world_pos();

    if(addFileInNextFrame) {
        add_file_to_canvas_by_path_execute(addFileInfo.first, addFileInfo.second);
        addFileInNextFrame = false;
    }

    drag_drop_update();

    if(world.main.input.key(InputManager::KEY_DRAW_TOOL_BRUSH).pressed)
        controls.selectedTool = TOOL_BRUSH;
    else if(world.main.input.key(InputManager::KEY_DRAW_TOOL_ERASER).pressed)
        controls.selectedTool = TOOL_ERASER;
    else if(world.main.input.key(InputManager::KEY_DRAW_TOOL_RECTSELECT).pressed)
        controls.selectedTool = TOOL_RECTSELECT;
    else if(world.main.input.key(InputManager::KEY_DRAW_TOOL_RECTANGLE).pressed)
        controls.selectedTool = TOOL_RECTANGLE;
    else if(world.main.input.key(InputManager::KEY_DRAW_TOOL_ELLIPSE).pressed)
        controls.selectedTool = TOOL_ELLIPSE;
    else if(world.main.input.key(InputManager::KEY_DRAW_TOOL_TEXTBOX).pressed)
        controls.selectedTool = TOOL_TEXTBOX;
    else if(world.main.input.key(InputManager::KEY_DRAW_TOOL_INKDROPPER).pressed)
        controls.selectedTool = TOOL_INKDROPPER;
    else if(world.main.input.key(InputManager::KEY_DRAW_TOOL_SCREENSHOT).pressed)
        controls.selectedTool = TOOL_SCREENSHOT;
    else if(world.main.input.key(InputManager::KEY_DRAW_TOOL_EDIT).pressed)
        controls.selectedTool = TOOL_EDIT;

    switch(controls.selectedTool) {
        case TOOL_BRUSH:
            brushTool.tool_update();
            break;
        case TOOL_ERASER:
            eraserTool.tool_update();
            break;
        case TOOL_RECTSELECT:
            rectSelectTool.tool_update();
            break;
        case TOOL_RECTANGLE:
            rectDrawTool.tool_update();
            break;
        case TOOL_ELLIPSE:
            ellipseDrawTool.tool_update();
            break;
        case TOOL_TEXTBOX:
            textBoxTool.tool_update();
            break;
        case TOOL_INKDROPPER:
            inkDropperTool.tool_update();
            break;
        case TOOL_SCREENSHOT:
            screenshotTool.tool_update();
            break;
        case TOOL_EDIT:
            editTool.tool_update();
            break;
        default:
            break;
    };

    if(controls.leftClickReleased)
        controls.leftClickReleased = false;

    check_delayed_update_timers();
    check_updateable_components();

    selection.update();

    if(controls.selectedTool == TOOL_ERASER)
        compCache.test_rebuild_dont_include_set_dont_include_nodes(components.client_list(), eraserTool.erasedComponents, eraserTool.erasedBVHNodes);
    else if(controls.selectedTool == TOOL_RECTSELECT)
        compCache.test_rebuild_dont_include_set(components.client_list(), selection.get_selected_set());
    else
        compCache.test_rebuild(components.client_list());
}

bool DrawingProgram::prevent_undo_or_redo() {
    switch(controls.selectedTool) {
        case TOOL_BRUSH:
            return brushTool.prevent_undo_or_redo();
        case TOOL_ERASER:
            return eraserTool.prevent_undo_or_redo();
        case TOOL_RECTSELECT:
            return rectSelectTool.prevent_undo_or_redo();
        case TOOL_RECTANGLE:
            return rectDrawTool.prevent_undo_or_redo();
        case TOOL_ELLIPSE:
            return ellipseDrawTool.prevent_undo_or_redo();
        case TOOL_TEXTBOX:
            return textBoxTool.prevent_undo_or_redo();
        case TOOL_INKDROPPER:
            return inkDropperTool.prevent_undo_or_redo();
        case TOOL_SCREENSHOT:
            return screenshotTool.prevent_undo_or_redo();
        case TOOL_EDIT:
            return editTool.prevent_undo_or_redo();
        default:
            break;
    };
    return false;
}

void DrawingProgram::force_rebuild_cache() {
    if(controls.selectedTool == TOOL_ERASER)
        compCache.test_rebuild_dont_include_set_dont_include_nodes(components.client_list(), eraserTool.erasedComponents, eraserTool.erasedBVHNodes, true);
    else if(controls.selectedTool == TOOL_RECTSELECT)
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
    img->final_update(*this);
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
    img->final_update(*this);
    uint64_t placement = components.client_list().size();
    auto objAdd = components.client_insert(placement, img);
    img->client_send_place(*this);
    add_undo_place_component(objAdd);
}

void DrawingProgram::reset_tools() {
    rectSelectTool.reset_tool();
    textBoxTool.reset_tool();
    brushTool.reset_tool();
    rectDrawTool.reset_tool();
    ellipseDrawTool.reset_tool();
    editTool.reset_tool();
    eraserTool.reset_tool();
}

void DrawingProgram::initialize_draw_data(cereal::PortableBinaryInputArchive& a) {
    uint64_t compCount;
    a(compCount);
    for(uint64_t i = 0; i < compCount; i++) {
        DrawComponentType t;
        ServerClientID id;
        a(t, id);
        auto newComp = DrawComponent::allocate_comp_type(t);
        a(newComp->coords, *newComp);
        components.init_emplace_back(id, newComp);
    }
    parallel_loop_all_components([&](const auto& c){
        c->obj->final_update(*this, false);
    });
    compCache.test_rebuild(components.client_list(), true);
}

void DrawingProgram::client_erase_set(std::unordered_set<CollabListType::ObjectInfoPtr> erasedComponents) {
    std::unordered_set<ServerClientID> idsToErase;
    for(auto& c : erasedComponents)
        idsToErase.emplace(c->id);
    DrawComponent::client_send_erase_set(*this, idsToErase);
    components.client_erase_set(erasedComponents);
    add_undo_erase_components(erasedComponents);
}

void DrawingProgram::add_undo_place_component(const CollabListType::ObjectInfoPtr& objToUndo) {
    world.undo.push(UndoManager::UndoRedoPair{
        [&, objToUndo]() {
            objToUndo->obj->client_send_erase(*this);
            components.client_erase(objToUndo);
            reset_tools();
            return true;
        },
        [&, objToUndo]() {
            if(objToUndo->obj->collabListInfo.lock())
                return false;
            components.client_insert(objToUndo);
            objToUndo->obj->client_send_place(*this);
            reset_tools();
            return true;
        }
    });
}

void DrawingProgram::add_undo_erase_components(const std::unordered_set<CollabListType::ObjectInfoPtr>& erasedComponents) {
    world.undo.push(UndoManager::UndoRedoPair{
        [&, erasedComponents = erasedComponents]() {
            std::vector<CollabListType::ObjectInfoPtr> sortedObjects(erasedComponents.begin(), erasedComponents.end());
            std::sort(sortedObjects.begin(), sortedObjects.end(), [](auto& a, auto& b) {
                return a->pos < b->pos;
            });

            for(auto& comp : sortedObjects)
                if(comp->obj->collabListInfo.lock())
                    return false;

            components.client_insert_ordered_vector(sortedObjects);
            DrawComponent::client_send_place_many(*this, sortedObjects);

            reset_tools();

            return true;
        },
        [&, erasedComponents = erasedComponents]() {
            for(auto& comp : erasedComponents)
                if(!comp->obj->collabListInfo.lock())
                    return false;

            std::unordered_set<ServerClientID> idsToErase;
            for(auto& c : erasedComponents)
                idsToErase.emplace(c->id);

            DrawComponent::client_send_erase_set(*this, idsToErase);
            components.client_erase_set(erasedComponents);

            reset_tools();

            return true;
        }
    });
}

ClientPortionID DrawingProgram::get_max_id(ServerPortionID serverID) {
    ClientPortionID maxClientID = 0;
    auto& cList = components.client_list();
    for(auto& comp : cList) {
        if(comp->id.first == serverID)
            maxClientID = std::max(maxClientID, comp->id.second);
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
        a(cList[i]->obj->get_type(), cList[i]->id, cList[i]->obj->coords, *cList[i]->obj);
    }
}

void DrawingProgram::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(drawData.dontUseDrawProgCache) {
        parallel_loop_all_components([&](auto& c) {
            c->obj->calculate_draw_transform(drawData);
        });
        for(auto& c : components.client_list())
            c->obj->draw(canvas, drawData);
    }
    else {
        if(world.main.drawProgCache.disableDrawCache) {
            parallel_loop_all_components([&](auto& c) {
                c->obj->calculate_draw_transform(drawData);
            });
            for(auto& c : components.client_list()) {
                c->obj->draw(canvas, drawData);
                c->obj->drawSetupData.shouldDraw = false;
            }
        }
        else {
            SkCanvas* drawProgCacheCanvas = world.main.drawProgCache.surface->getCanvas();
            drawProgCacheCanvas->clear(SkColor4f{0.0f, 0.0f, 0.0f, 0.0f});
            compCache.refresh_all_draw_cache(drawData);
            compCache.draw_components_to_canvas(drawProgCacheCanvas, drawData);
            drawProgCacheCanvas->saveLayerAlphaf(nullptr, 1.0f);
            selection.draw_components(drawProgCacheCanvas, drawData);
            drawProgCacheCanvas->restore();
            canvas->drawImage(world.main.drawProgCache.surface->makeTemporaryImage(), 0, 0);
            selection.draw_gui(canvas, drawData);
        }
    }

    switch(controls.selectedTool) {
        case TOOL_BRUSH:
            brushTool.draw(canvas, drawData);
            break;
        case TOOL_ERASER:
            eraserTool.draw(canvas, drawData);
            break;
        case TOOL_RECTSELECT:
            rectSelectTool.draw(canvas, drawData);
            break;
        case TOOL_RECTANGLE:
            rectDrawTool.draw(canvas, drawData);
            break;
        case TOOL_ELLIPSE:
            ellipseDrawTool.draw(canvas, drawData);
            break;
        case TOOL_TEXTBOX:
            textBoxTool.draw(canvas, drawData);
            break;
        case TOOL_INKDROPPER:
            inkDropperTool.draw(canvas, drawData);
            break;
        case TOOL_SCREENSHOT:
            screenshotTool.draw(canvas, drawData);
            break;
        case TOOL_EDIT:
            editTool.draw(canvas, drawData);
            break;
        default:
            break;
    }
}

Vector4f* DrawingProgram::get_foreground_color_ptr() {
    return &controls.foregroundColor;
}
