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

#pragma once
#include "../CanvasComponents/CanvasComponentContainer.hpp"

class EditCanvasComponentWorldUndoAction : public WorldUndoAction {
    public:
        EditCanvasComponentWorldUndoAction(std::unique_ptr<CanvasComponent> initData, WorldUndoManager::UndoObjectID initUndoID);
        std::string get_name() const override;
        bool undo(WorldUndoManager& undoMan) override;
        bool redo(WorldUndoManager& undoMan) override;
        bool undo_redo(WorldUndoManager& undoMan);
        ~EditCanvasComponentWorldUndoAction();

        std::unique_ptr<CanvasComponent> data;
        WorldUndoManager::UndoObjectID undoID;
};
