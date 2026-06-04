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
#include <memory>
#include <Helpers/NetworkingObjects/NetObjID.hpp>
#include <unordered_map>
#include <deque>

class World;
class WorldUndoManager;

class WorldUndoAction {
    public:
        virtual std::string get_name() const = 0;
        virtual bool undo(WorldUndoManager& undoMan) = 0;
        virtual bool redo(WorldUndoManager& undoMan) = 0;
        virtual void scale_up(const WorldScalar& scaleAmount);
        virtual ~WorldUndoAction();
};

class WorldUndoManager {
    public:
        typedef uint64_t UndoObjectID;
        WorldUndoManager(World& initWorld);
        
        void push(std::unique_ptr<WorldUndoAction> undoAction);
        void push_on_last(std::unique_ptr<WorldUndoAction> undoAction);
        void undo();
        void redo();
        void clear();
        bool can_undo();
        bool can_redo();
        void scale_up(const WorldScalar& scaleAmount);

        void reassign_netid(const NetworkingObjects::NetObjID& oldNetObjID, const NetworkingObjects::NetObjID& newNetObjID);
        void remove_by_netid(const NetworkingObjects::NetObjID& netObjID);
        void remove_by_undoid(UndoObjectID undoID);
        UndoObjectID get_undoid_from_netid(const NetworkingObjects::NetObjID& netObjID);
        void register_new_netid_to_existing_undoid(UndoObjectID existingUndoID, const NetworkingObjects::NetObjID& netObjID);
        std::optional<NetworkingObjects::NetObjID> get_netid_from_undoid(UndoObjectID undoID);
        bool fill_netid_list_from_undoid_list(std::vector<NetworkingObjects::NetObjID>& netIDList, const std::vector<UndoObjectID>& undoIDList);
        std::vector<std::string> get_front_undo_queue_names(unsigned count);

        void set_save_action();

        World& world;
    private:
        void push_undo(std::vector<std::unique_ptr<WorldUndoAction>> undoAction);
        void push_redo(std::vector<std::unique_ptr<WorldUndoAction>> undoAction);
        constexpr static size_t UNDO_QUEUE_LIMIT = 250;

        std::unordered_map<UndoObjectID, NetworkingObjects::NetObjID> undoIDToNetID;
        std::unordered_map<NetworkingObjects::NetObjID, UndoObjectID> netIDToUndoID;
        std::deque<std::vector<std::unique_ptr<WorldUndoAction>>> undoQueue;
        std::deque<std::vector<std::unique_ptr<WorldUndoAction>>> redoQueue;

        std::optional<WorldUndoAction*> undoActionSavedAt = nullptr;

        void set_world_has_unsaved_local_changes();

        UndoObjectID lastUndoObjectID = 0;
};
