/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "GridManager.hpp"
#include "World.hpp"
#include "MainProgram.hpp"
#include "WorldUndoManager.hpp"
#include "GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "GUIStuff/ElementHelpers/LayoutHelpers.hpp"
#include "GUIStuff/ElementHelpers/ButtonHelpers.hpp"
#include "GUIStuff/ElementHelpers/TextBoxHelpers.hpp"
#include "GUIStuff/Elements/ManyElementScrollArea.hpp"
#include "GUIStuff/Elements/LayoutElement.hpp"
#include "GUIStuff/Elements/SelectableButton.hpp"
#include <Helpers/NetworkingObjects/NetObjGenericSerializedClass.hpp>
#include "WorldGrid.hpp"
#include <cereal/types/unordered_map.hpp>

GridManager::GridManager(World& w):
    world(w) {}

void GridManager::server_init_no_file() {
    grids = world.netObjMan.make_obj<NetworkingObjects::NetObjOrderedList<WorldGrid>>();
    set_grid_list_callbacks();
}

void GridManager::read_create_message(cereal::PortableBinaryInputArchive& a) {
    grids = world.netObjMan.read_create_message<NetworkingObjects::NetObjOrderedList<WorldGrid>>(a, nullptr);
    set_grid_list_callbacks();
}

void GridManager::set_grid_list_callbacks() {
    grids->set_insert_callback([&](auto&) { world.set_to_layout_gui_if_focus(); });
    grids->set_erase_callback([&](auto&) { world.set_to_layout_gui_if_focus(); });
    grids->set_move_callback([&](auto&, uint32_t) { world.set_to_layout_gui_if_focus(); });
}

void GridManager::add_default_grid(const std::string& newName) {
    if(grids) {
        WorldGrid g;
        g.name = newName;
        g.color = color_mul_alpha(convert_vec4<Vector4f>(world.canvasTheme.get_tool_front_color()), 0.4f);
        g.size = world.drawData.cam.c.inverseScale * WorldScalar(WorldGrid::GRID_UNIT_PIXEL_SIZE);
        g.offset = world.drawData.cam.c.pos + world.drawData.cam.c.dir_from_space(world.main.window.size.cast<float>() * 0.5f);
        auto it = grids->emplace_back_direct(grids, g);

        class AddGridWorldUndoAction : public WorldUndoAction {
            public:
                AddGridWorldUndoAction(std::unique_ptr<WorldGrid> initGridData, uint32_t newPos, WorldUndoManager::UndoObjectID initUndoID):
                    gridData(std::move(initGridData)),
                    pos(newPos),
                    undoID(initUndoID)
                {}
                std::string get_name() const override {
                    return "Add Grid";
                }
                bool undo(WorldUndoManager& undoMan) override {
                    std::optional<NetworkingObjects::NetObjID> toEraseID = undoMan.get_netid_from_undoid(undoID);
                    if(!toEraseID.has_value())
                        return false;
                    auto& gridList = undoMan.world.gridMan.grids;
                    auto it = gridList->get(toEraseID.value());
                    gridData = std::make_unique<WorldGrid>(*it->obj);
                    gridList->erase(gridList, it);
                    return true;
                }
                bool redo(WorldUndoManager& undoMan) override {
                    std::optional<NetworkingObjects::NetObjID> toInsertID = undoMan.get_netid_from_undoid(undoID);
                    if(toInsertID.has_value())
                        return false;
                    auto& gridList = undoMan.world.gridMan.grids;
                    auto insertedIt = gridList->emplace_direct(gridList, gridList->at(pos), *gridData);
                    undoMan.register_new_netid_to_existing_undoid(undoID, insertedIt->obj.get_net_id());
                    gridData = nullptr;
                    return true;
                }
                void scale_up(const WorldScalar& scaleAmount) override {
                    if(gridData)
                        gridData->scale_up(scaleAmount);
                }
                ~AddGridWorldUndoAction() {}

                std::unique_ptr<WorldGrid> gridData;
                uint32_t pos;
                WorldUndoManager::UndoObjectID undoID;
        };

        world.undo.push(std::make_unique<AddGridWorldUndoAction>(std::make_unique<WorldGrid>(*it->obj), it->pos, world.undo.get_undoid_from_netid(it->obj.get_net_id())));
    }
}

void GridManager::remove_grid(uint32_t indexToRemove) {
    if(grids) {
        class DeleteGridWorldUndoAction : public WorldUndoAction {
            public:
                DeleteGridWorldUndoAction(std::unique_ptr<WorldGrid> initGridData, uint32_t newPos, WorldUndoManager::UndoObjectID initUndoID):
                    gridData(std::move(initGridData)),
                    pos(newPos),
                    undoID(initUndoID)
                {}
                std::string get_name() const override {
                    return "Delete Grid";
                }
                bool undo(WorldUndoManager& undoMan) override {
                    std::optional<NetworkingObjects::NetObjID> toInsertID = undoMan.get_netid_from_undoid(undoID);
                    if(toInsertID.has_value())
                        return false;
                    auto& gridList = undoMan.world.gridMan.grids;
                    auto insertedIt = gridList->emplace_direct(gridList, gridList->at(pos), *gridData);
                    undoMan.register_new_netid_to_existing_undoid(undoID, insertedIt->obj.get_net_id());
                    gridData = nullptr;
                    return true;
                }
                bool redo(WorldUndoManager& undoMan) override {
                    std::optional<NetworkingObjects::NetObjID> toEraseID = undoMan.get_netid_from_undoid(undoID);
                    if(!toEraseID.has_value())
                        return false;
                    auto& gridList = undoMan.world.gridMan.grids;
                    auto it = gridList->get(toEraseID.value());
                    gridData = std::make_unique<WorldGrid>(*it->obj);
                    gridList->erase(gridList, it);
                    return true;
                }
                void scale_up(const WorldScalar& scaleAmount) override {
                    if(gridData)
                        gridData->scale_up(scaleAmount);
                }
                ~DeleteGridWorldUndoAction() {}

                std::unique_ptr<WorldGrid> gridData;
                uint32_t pos;
                WorldUndoManager::UndoObjectID undoID;
        };

        auto it = grids->at(indexToRemove);
        world.undo.push(std::make_unique<DeleteGridWorldUndoAction>(std::make_unique<WorldGrid>(*it->obj), it->pos, world.undo.get_undoid_from_netid(it->obj.get_net_id())));
        grids->erase(grids, it);
    }
}

void GridManager::finalize_grid_modify(const NetworkingObjects::NetObjTemporaryPtr<WorldGrid>& wGrid, const WorldGrid& oldGridData) {
    class EditGridWorldUndoAction : public WorldUndoAction {
        public:
            EditGridWorldUndoAction(std::unique_ptr<WorldGrid> initGridData, WorldUndoManager::UndoObjectID initUndoID):
                gridData(std::move(initGridData)),
                undoID(initUndoID)
            {}
            std::string get_name() const override {
                return "Edit Grid";
            }
            bool undo(WorldUndoManager& undoMan) override {
                return undo_redo(undoMan);
            }
            bool redo(WorldUndoManager& undoMan) override {
                return undo_redo(undoMan);
            }
            bool undo_redo(WorldUndoManager& undoMan) {
                std::optional<NetworkingObjects::NetObjID> toEditID = undoMan.get_netid_from_undoid(undoID);
                if(!toEditID.has_value())
                    return false;
                auto gridPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<WorldGrid>(toEditID.value());
                auto newGrid = std::make_unique<WorldGrid>(*gridPtr);
                *gridPtr = *gridData;
                gridData = std::move(newGrid);
                undoMan.world.delayedUpdateObjectManager.send_update_to_all<WorldGrid>(gridPtr, true);
                return true;
            }
            void scale_up(const WorldScalar& scaleAmount) override {
                gridData->scale_up(scaleAmount);
            }
            ~EditGridWorldUndoAction() {}

            std::unique_ptr<WorldGrid> gridData;
            WorldUndoManager::UndoObjectID undoID;
    };

    world.undo.push(std::make_unique<EditGridWorldUndoAction>(std::make_unique<WorldGrid>(oldGridData), world.undo.get_undoid_from_netid(wGrid.get_net_id())));
    world.delayedUpdateObjectManager.send_update_to_all<WorldGrid>(wGrid, true);
}

void GridManager::draw_back(SkCanvas* canvas, const DrawData& drawData) {
    if(grids) {
        for(uint32_t i = 0; i < grids->size(); i++) {
            auto& g = grids->at(i)->obj;
            if(!g->displayInFront)
                g->draw(*this, canvas, drawData);
        }
    }
}

void GridManager::draw_front(SkCanvas* canvas, const DrawData& drawData) {
    if(grids) {
        for(uint32_t i = 0; i < grids->size(); i++) {
            auto& g = grids->at(i)->obj;
            if(g->displayInFront)
                g->draw(*this, canvas, drawData);
        }
    }
}

void GridManager::draw_coordinates(SkCanvas* canvas, const DrawData& drawData) {
    if(grids) {
        Vector2f axisOffset{0.0f, 0.0f};
        for(uint32_t i = 0; i < grids->size(); i++) {
            auto& g = grids->at(i)->obj;
            if(g->coordinatesWillBeDrawn && g->coordinatesAxisOnBounds) {
                g->draw_coordinates(canvas, drawData, axisOffset);
                g->coordinatesWillBeDrawn = false;
            }
        }
        for(uint32_t i = 0; i < grids->size(); i++) {
            auto& g = grids->at(i)->obj;
            if(g->coordinatesWillBeDrawn && !g->coordinatesAxisOnBounds)
                g->draw_coordinates(canvas, drawData, axisOffset);
            g->coordinatesWillBeDrawn = false;
        }
    }
}

void GridManager::scale_up(const WorldScalar& scaleUpAmount) {
    if(grids) {
        for(uint32_t i = 0; i < grids->size(); i++)
            grids->at(i)->obj->scale_up(scaleUpAmount);
    }
}

void GridManager::save_file(cereal::PortableBinaryOutputArchive& a) const {
    a(grids->size());
    for(auto& item : *grids)
        a(*item.obj);
}

void GridManager::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    server_init_no_file();
    if(version >= VersionNumber(0, 4, 0)) {
        uint32_t gridSize;
        a(gridSize);
        for(uint32_t i = 0; i < gridSize; i++) {
            WorldGrid* g = new WorldGrid;
            a(*g);
            grids->push_back_and_send_create(grids, g);
        }
    }
    else if(version >= VersionNumber(0, 2, 0)) {
        std::unordered_map<NetworkingObjects::NetObjID, WorldGrid> gridMap;
        a(gridMap);
        for(auto& [netID, wGrid] : gridMap)
            grids->emplace_back_direct(grids, wGrid);
    }
}

void GridManager::setup_list_gui(const std::function<void()>& onStartModify) {
    auto& gui = world.main.g.gui;
    auto& main = world.main;

    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto add_grid = [&, onStartModify] {
        if(!gridMenu.newName.empty()) {
            main.world->gridMan.add_default_grid(gridMenu.newName);
            main.world->drawProg.modify_grid(main.world->gridMan.grids->at(main.world->gridMan.grids->size() - 1)->obj);
            if(onStartModify) onStartModify();
        }
    };

    float ENTRY_HEIGHT = 25.0f;
    if(main.world->gridMan.grids->empty())
        text_label_centered(gui, "No grids exist.");
    gui.element<ManyElementScrollArea>("grid menu entries", ManyElementScrollArea::Options{
        .entryHeight = ENTRY_HEIGHT,
        .entryCount = main.world->gridMan.grids->size(),
        .clipHorizontal = true,
        .elementContent = [&, onStartModify](size_t i) {
            auto& grid = main.world->gridMan.grids->at(i)->obj;
            bool selectedEntry = gridMenu.selectedGrid == i;
            gui.element<LayoutElement>("elem", [&, onStartModify] (LayoutElement*, const Clay_ElementId& lId2) {
                CLAY(lId2, {
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(ENTRY_HEIGHT)},
                        .childGap = 2,
                        .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                        .layoutDirection = CLAY_LEFT_TO_RIGHT 
                    },
                    .backgroundColor = selectedEntry ? convert_vec4<Clay_Color>(gui.io.theme->backColor1) : convert_vec4<Clay_Color>(gui.io.theme->backColor2)
                }) {
                    text_label(gui, grid->get_display_name());
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            .childGap = 1,
                            .childAlignment = {.x = CLAY_ALIGN_X_RIGHT, .y = CLAY_ALIGN_Y_CENTER},
                            .layoutDirection = CLAY_LEFT_TO_RIGHT
                        }
                    }) {
                        auto list_button = [&](const char* id, const char* svgPath, const std::function<void()>& onClick) {
                            gui.set_z_index_keep_clipping_region(gui.get_z_index() + 1, [&] {
                                svg_icon_button(gui, id, svgPath, {
                                    .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                                    .size = ENTRY_HEIGHT,
                                    .onClick = onClick
                                });
                            });
                        };
                        list_button("visibility eye", grid->visible ? "data/icons/eyeopen.svg" : "data/icons/eyeclose.svg", [&] {
                            grid->visible = !grid->visible;
                            NetworkingObjects::generic_serialized_class_send_update_to_all<WorldGrid>(grid);
                        });
                        list_button("edit pencil", "data/icons/pencil.svg", [&, onStartModify] {
                            main.world->drawProg.modify_grid(grid);
                            if(onStartModify) onStartModify();
                        });
                        list_button("delete trash", "data/icons/trash.svg", [&, i] {
                            main.world->gridMan.remove_grid(i);
                        });
                        // Gap for scroll bar
                        CLAY_AUTO_ID({
                            .layout = {.sizing = {.width = CLAY_SIZING_FIT(15), .height = CLAY_SIZING_GROW(0)}}
                        }) {}
                    }
                }
            }, LayoutElement::Callbacks {
                .onClick = [&, i, onStartModify] (LayoutElement* l, const InputManager::MouseButtonCallbackArgs& button) {
                    if(l->mouseHovering && button.down && button.button == InputManager::MouseButton::LEFT) {
                        gridMenu.selectedGrid = i;
                        if(button.clicks == 2) {
                            main.world->drawProg.modify_grid(grid);
                            if(onStartModify) onStartModify();
                        }
                        gui.set_to_layout();
                    }
                }
            });
        }
    });
    left_to_right_line_layout(gui, [&]() {
        input_text(gui, "grid text input", &gridMenu.newName, {
            .onEnter = [add_grid]() { add_grid(); }
        });
        svg_icon_button(gui, "grid add button", "data/icons/plusbold.svg", {
            .size = SMALL_BUTTON_SIZE,
            .onClick = [add_grid] { add_grid(); }
        });
    });
}

void GridManager::refresh_gui_data() {
    gridMenu.newName.clear();
    gridMenu.selectedGrid = std::numeric_limits<uint32_t>::max();
}
