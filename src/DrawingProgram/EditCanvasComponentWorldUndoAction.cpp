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

#include "EditCanvasComponentWorldUndoAction.hpp"
#include "../CanvasComponents/CanvasComponent.hpp"
#include "../World.hpp"

EditCanvasComponentWorldUndoAction::EditCanvasComponentWorldUndoAction(std::unique_ptr<CanvasComponent> initData, WorldUndoManager::UndoObjectID initUndoID):
    data(std::move(initData)),
    undoID(initUndoID)
{}

std::string EditCanvasComponentWorldUndoAction::get_name() const {
    return "Edit Canvas Component";
}

bool EditCanvasComponentWorldUndoAction::undo(WorldUndoManager& undoMan) {
    return undo_redo(undoMan);
}

bool EditCanvasComponentWorldUndoAction::redo(WorldUndoManager& undoMan) {
    return undo_redo(undoMan);
}

bool EditCanvasComponentWorldUndoAction::undo_redo(WorldUndoManager& undoMan) {
    std::optional<NetworkingObjects::NetObjID> toEditID = undoMan.get_netid_from_undoid(undoID);
    if(!toEditID.has_value())
        return false;
    auto objPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<CanvasComponentContainer>(toEditID.value());
    std::unique_ptr<CanvasComponent> newData = objPtr->get_comp().get_data_copy();
    objPtr->get_comp().set_data_from(*data);
    data = std::move(newData);
    objPtr->commit_update(undoMan.world.drawProg);
    objPtr->send_comp_update(undoMan.world.drawProg, true);
    return true;
}

EditCanvasComponentWorldUndoAction::~EditCanvasComponentWorldUndoAction() {}




EditTransformCanvasComponentWorldUndoAction::EditTransformCanvasComponentWorldUndoAction(std::unique_ptr<CanvasComponent> initData, const CoordSpaceHelper& initCoords, WorldUndoManager::UndoObjectID initUndoID):
    data(std::move(initData)),
    coords(initCoords),
    undoID(initUndoID)
{}

std::string EditTransformCanvasComponentWorldUndoAction::get_name() const {
    return "Edit Canvas Component With Transform";
}

bool EditTransformCanvasComponentWorldUndoAction::undo(WorldUndoManager& undoMan) {
    return undo_redo(undoMan);
}

bool EditTransformCanvasComponentWorldUndoAction::redo(WorldUndoManager& undoMan) {
    return undo_redo(undoMan);
}

bool EditTransformCanvasComponentWorldUndoAction::undo_redo(WorldUndoManager& undoMan) {
    std::optional<NetworkingObjects::NetObjID> toEditID = undoMan.get_netid_from_undoid(undoID);
    if(!toEditID.has_value())
        return false;
    auto objPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<CanvasComponentContainer>(toEditID.value());
    std::unique_ptr<CanvasComponent> newData = objPtr->get_comp().get_data_copy();
    objPtr->get_comp().set_data_from(*data);
    data = std::move(newData);
    std::swap(coords, objPtr->coords);
    undoMan.world.drawProg.send_transforms_for({&(*objPtr->objInfo)});
    objPtr->commit_update(undoMan.world.drawProg);
    objPtr->send_comp_update(undoMan.world.drawProg, true);
    return true;
}

void EditTransformCanvasComponentWorldUndoAction::scale_up(const WorldScalar& scaleAmount) {
    coords.scale_about(WorldVec{0, 0}, scaleAmount, true);
}

EditTransformCanvasComponentWorldUndoAction::~EditTransformCanvasComponentWorldUndoAction() {}
