#include "GridManager.hpp"
#include "World.hpp"
#include "MainProgram.hpp"
#include "WorldUndoManager.hpp"
#include <cereal/types/unordered_map.hpp>

GridManager::GridManager(World& w):
    world(w) {}

void GridManager::server_init_no_file() {
    grids = world.netObjMan.make_obj<NetworkingObjects::NetObjOrderedList<WorldGrid>>();
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
                AddGridWorldUndoAction(const WorldGrid& initGridData, uint32_t newPos, WorldUndoManager::UndoObjectID initUndoID):
                    gridData(initGridData),
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
                    gridList->erase(gridList, gridList->get(toEraseID.value()));
                    return true;
                }
                bool redo(WorldUndoManager& undoMan) override {
                    std::optional<NetworkingObjects::NetObjID> toInsertID = undoMan.get_netid_from_undoid(undoID);
                    if(toInsertID.has_value())
                        return false;
                    auto& gridList = undoMan.world.gridMan.grids;
                    auto insertedIt = gridList->emplace_direct(gridList, gridList->at(pos), gridData);
                    undoMan.register_new_netid_to_existing_undoid(undoID, insertedIt->obj.get_net_id());
                    return true;
                }
                void scale_up(const WorldScalar& scaleAmount) override {
                    gridData.scale_up(scaleAmount);
                }
                ~AddGridWorldUndoAction() {}

                WorldGrid gridData;
                uint32_t pos;
                WorldUndoManager::UndoObjectID undoID;
        };

        world.undo.push(std::make_unique<AddGridWorldUndoAction>(g, it->pos, world.undo.get_undoid_from_netid(it->obj.get_net_id())));
    }
}

void GridManager::remove_grid(uint32_t indexToRemove) {
    if(grids) {
        class DeleteGridWorldUndoAction : public WorldUndoAction {
            public:
                DeleteGridWorldUndoAction(const WorldGrid& initGridData, uint32_t newPos, WorldUndoManager::UndoObjectID initUndoID):
                    gridData(initGridData),
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
                    auto insertedIt = gridList->emplace_direct(gridList, gridList->at(pos), gridData);
                    undoMan.register_new_netid_to_existing_undoid(undoID, insertedIt->obj.get_net_id());
                    return true;
                }
                bool redo(WorldUndoManager& undoMan) override {
                    std::optional<NetworkingObjects::NetObjID> toEraseID = undoMan.get_netid_from_undoid(undoID);
                    if(!toEraseID.has_value())
                        return false;
                    auto& gridList = undoMan.world.gridMan.grids;
                    gridList->erase(gridList, gridList->get(toEraseID.value()));
                    return true;
                }
                void scale_up(const WorldScalar& scaleAmount) override {
                    gridData.scale_up(scaleAmount);
                }
                ~DeleteGridWorldUndoAction() {}

                WorldGrid gridData;
                uint32_t pos;
                WorldUndoManager::UndoObjectID undoID;
        };

        auto it = grids->at(indexToRemove);
        world.undo.push(std::make_unique<DeleteGridWorldUndoAction>(*it->obj, it->pos, world.undo.get_undoid_from_netid(it->obj.get_net_id())));
        grids->erase(grids, it);
    }
}

void GridManager::finalize_grid_modify(const NetworkingObjects::NetObjTemporaryPtr<WorldGrid>& wGrid, const WorldGrid& oldGridData) {
    class EditGridWorldUndoAction : public WorldUndoAction {
        public:
            EditGridWorldUndoAction(const WorldGrid& initGridDataOld, const WorldGrid& initGridDataNew, WorldUndoManager::UndoObjectID initUndoID):
                gridDataOld(initGridDataOld),
                gridDataNew(initGridDataNew),
                undoID(initUndoID)
            {}
            std::string get_name() const override {
                return "Edit Grid";
            }
            bool undo(WorldUndoManager& undoMan) override {
                std::optional<NetworkingObjects::NetObjID> toEditID = undoMan.get_netid_from_undoid(undoID);
                if(!toEditID.has_value())
                    return false;
                auto gridPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<WorldGrid>(toEditID.value());
                *gridPtr = gridDataOld;
                undoMan.world.delayedUpdateObjectManager.send_update_to_all<WorldGrid>(gridPtr, true);
                return true;
            }
            bool redo(WorldUndoManager& undoMan) override {
                std::optional<NetworkingObjects::NetObjID> toEditID = undoMan.get_netid_from_undoid(undoID);
                if(!toEditID.has_value())
                    return false;
                auto gridPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<WorldGrid>(toEditID.value());
                *gridPtr = gridDataNew;
                undoMan.world.delayedUpdateObjectManager.send_update_to_all<WorldGrid>(gridPtr, true);
                return true;
            }
            void scale_up(const WorldScalar& scaleAmount) override {
                gridDataOld.scale_up(scaleAmount);
                gridDataNew.scale_up(scaleAmount);
            }
            ~EditGridWorldUndoAction() {}

            WorldGrid gridDataOld;
            WorldGrid gridDataNew;
            WorldUndoManager::UndoObjectID undoID;
    };

    world.undo.push(std::make_unique<EditGridWorldUndoAction>(oldGridData, *wGrid, world.undo.get_undoid_from_netid(wGrid.get_net_id())));
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
    grids = world.netObjMan.make_obj<NetworkingObjects::NetObjOrderedList<WorldGrid>>();
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
