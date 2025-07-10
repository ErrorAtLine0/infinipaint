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
#include <execution>
#ifdef __APPLE__
    #define POOLSTL_STD_SUPPLEMENT
    #include <poolstl.hpp>
#endif

DrawingProgram::DrawingProgram(World& initWorld):
    world(initWorld),
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
    imageTool(*this)
{
    components.updateCallback = [&]() {
        if(&world == world.main.world.get())
            world.main.drawProgCache.refresh = true;
    };
}

void DrawingProgram::init_client_callbacks() {
    world.con.client_add_recv_callback(CLIENT_UPDATE_COMPONENT, [&](cereal::PortableBinaryInputArchive& message) {
        bool isTemp;
        ServerClientID id;
        message(isTemp, id);
        std::shared_ptr<DrawComponent>* compPtr = components.get_item_by_id(id);
        if(!compPtr)
            return;
        std::shared_ptr<DrawComponent>& comp = *compPtr;
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
        }
    });
    world.con.client_add_recv_callback(CLIENT_TRANSFORM_COMPONENT, [&](cereal::PortableBinaryInputArchive& message) {
        bool isTemp;
        ServerClientID id;
        message(isTemp, id);
        std::shared_ptr<DrawComponent>* compPtr = components.get_item_by_id(id);
        if(!compPtr)
            return;
        std::shared_ptr<DrawComponent>& comp = *compPtr;
        float dur = std::chrono::duration_cast<std::chrono::duration<float>>(std::chrono::steady_clock::now() - comp->lastTransformTime).count();
        if(dur > CLIENT_DRAWCOMP_DELAY_TIMER_DURATION){
            comp->delayedCoordinateSpace = nullptr;
            comp->updateDraw = true;
            message(comp->coords);
            if(!isTemp)
                comp->finalize_update(colliderAllocated); // No need to update draw data during transformation
        }
        else {
            comp->delayedCoordinateSpace = std::make_shared<CoordSpaceHelper>();
            message(*comp->delayedCoordinateSpace);
        }
    });
    world.con.client_add_recv_callback(CLIENT_PLACE_COMPONENT, [&](cereal::PortableBinaryInputArchive& message) {
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
    world.con.client_add_recv_callback(CLIENT_ERASE_COMPONENT, [&](cereal::PortableBinaryInputArchive& message) {
        ServerClientID cToRemove;
        message(cToRemove);
        components.server_erase(cToRemove);
    });
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

void DrawingProgram::check_all_collisions_base(const SCollision::ColliderCollection<WorldScalar>& checkAgainstWorld, const SCollision::ColliderCollection<float>& checkAgainstCam) {
#ifdef __EMSCRIPTEN__
    std::for_each(components.client_list().begin(), components.client_list().end(), [&](auto& c) {
        c->obj->globalCollisionCheck = c->obj->collides_with(world.drawData.cam.c, checkAgainstWorld, checkAgainstCam, colliderAllocated);
    });
#else
    std::for_each(std::execution::par_unseq, components.client_list().begin(), components.client_list().end(), [&](auto& c) {
        c->obj->globalCollisionCheck = c->obj->collides_with(world.drawData.cam.c, checkAgainstWorld, checkAgainstCam, colliderAllocated);
    });
#endif
}

void DrawingProgram::check_all_collisions(const SCollision::ColliderCollection<WorldScalar>& checkAgainst) {
    check_all_collisions_base(checkAgainst, world.drawData.cam.c.world_collider_to_coords<SCollision::ColliderCollection<float>>(checkAgainst));
}

void DrawingProgram::check_all_collisions_transform(const SCollision::ColliderCollection<float>& checkAgainst) {
    check_all_collisions_base(world.drawData.cam.c.collider_to_world<SCollision::ColliderCollection<WorldScalar>, SCollision::ColliderCollection<float>>(checkAgainst), checkAgainst);
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
    else if(world.main.input.key(InputManager::KEY_PASTE).pressed)
        rectSelectTool.paste_clipboard();

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

    for(auto& c : components.client_list()) {
        c->obj->check_timers(*this);
        c->obj->update(*this);
    }
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
    img->initialize_draw_data(*this);
    img->finalize_update(colliderAllocated);
    uint64_t placement = components.client_list().size();
    components.client_insert(placement, img);
    img->client_send_place(*this, placement);
    add_undo_place_component(placement, img);
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
    img->initialize_draw_data(*this);
    img->finalize_update(colliderAllocated);
    uint64_t placement = components.client_list().size();
    components.client_insert(placement, img);
    img->client_send_place(*this, placement);
    add_undo_place_component(placement, img);
}

void DrawingProgram::reset_tools() {
    rectSelectTool.reset_tool();
    textBoxTool.reset_tool();
    brushTool.reset_tool();
    rectDrawTool.reset_tool();
    ellipseDrawTool.reset_tool();
    editTool.reset_tool();
    reset_selection();
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
        newComp->initialize_draw_data(*this);
        newComp->finalize_update(colliderAllocated);
    }
}

void DrawingProgram::reset_selection() {
    for(auto& c : components.client_list())
        c->obj->selected = false;
}

void DrawingProgram::add_undo_place_component(uint64_t placement, const std::shared_ptr<DrawComponent>& comp) {
    world.undo.push(UndoManager::UndoRedoPair{
        [&, comp]() {
            ServerClientID compID;
            components.client_erase(comp, compID);
            if(compID == ServerClientID{0, 0})
                return false;
            DrawComponent::client_send_erase(*this, compID);
            reset_tools();
            return true;
        },
        [&, comp, placement]() {
            if(components.get_id(comp) != ServerClientID{0, 0})
                return false;
            components.client_insert(placement, comp);
            comp->initialize_draw_data(*this); // Method to recover after an undo that happened while drawing a component
            comp->finalize_update(colliderAllocated);
            comp->client_send_place(*this, placement);
            reset_tools();
            return true;
        }
    });
}

void DrawingProgram::add_undo_place_components(uint64_t placement, const std::vector<std::shared_ptr<DrawComponent>>& comps) {
    world.undo.push(UndoManager::UndoRedoPair{
        [&, comps]() {
            std::vector<ServerClientID> compIDs;
            for(auto& c : comps) {
                ServerClientID compID;
                components.client_erase(c, compID);
                compIDs.emplace_back(compID);
            }
            bool toRet = true;
            for(auto& compID : compIDs) {
                if(compID == ServerClientID{0, 0})
                    toRet = false;
                else
                    DrawComponent::client_send_erase(*this, compID);
            }
            reset_tools();
            return toRet;
        },
        [&, comps, placement]() {
            for(auto& c : comps) 
                if(components.get_id(c) != ServerClientID{0, 0})
                    return false;
            for(auto& c : comps | std::views::reverse) {
                components.client_insert(placement, c);
                c->initialize_draw_data(*this); // Method to recover after an undo that happened while drawing a component
                c->finalize_update(colliderAllocated);
                c->client_send_place(*this, placement);
            }
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
        for(auto& c : components.client_list()) {
            c->obj->calculate_draw_transform(drawData);
            c->obj->draw(canvas, drawData);
        }
    }
    else if(world.main.drawProgCache.refresh)
        draw_cache(canvas, drawData, nullptr);
    else {
        bool caughtFirstUpdated = false;
        for(auto& c : components.client_list()) {
            if(caughtFirstUpdated) {
                c->obj->calculate_draw_transform(drawData);
                c->obj->draw(canvas, drawData);
            }
            else if(c->obj == world.main.drawProgCache.firstCompUpdate) {
                canvas->drawImage(world.main.drawProgCache.surface->makeTemporaryImage(), 0, 0);
                c->obj->calculate_draw_transform(drawData);
                c->obj->draw(canvas, drawData);
                caughtFirstUpdated = true;
            }
            else if(c->obj->updateDraw) {
                draw_cache(canvas, drawData, c->obj);
                c->obj->calculate_draw_transform(drawData);
                c->obj->draw(canvas, drawData);
                caughtFirstUpdated = true;
            }
            c->obj->updateDraw = false;
        }
        if(!caughtFirstUpdated)
            canvas->drawImage(world.main.drawProgCache.surface->makeTemporaryImage(), 0, 0);
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

void DrawingProgram::draw_cache(SkCanvas* canvas, const DrawData& drawData, const std::shared_ptr<DrawComponent>& lastComp) {
    SkCanvas* cacheCanvas = world.main.drawProgCache.canvas;
    cacheCanvas->clear(SkColor4f{0, 0, 0, 0});
#ifdef __EMSCRIPTEN__
    std::for_each(components.client_list().begin(), components.client_list().end(), [&](auto& c) {
        c->obj->calculate_draw_transform(drawData);
    });
#else
    std::for_each(std::execution::par_unseq, components.client_list().begin(), components.client_list().end(), [&](auto& c) {
        c->obj->calculate_draw_transform(drawData);
    });
#endif
    for(auto& c : components.client_list()) {
        if(c->obj == lastComp)
            break;
        c->obj->draw(cacheCanvas, drawData);
    }
    canvas->drawImage(world.main.drawProgCache.surface->makeTemporaryImage(), 0, 0);
    world.main.drawProgCache.firstCompUpdate = lastComp;
    world.main.drawProgCache.refresh = false;
}
