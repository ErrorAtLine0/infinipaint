#pragma once
#include <memory>
#include <Helpers/NetworkingObjects/NetObjID.hpp>
#include <unordered_map>
#include <deque>

class World;
class WorldUndoManager;

class WorldUndoAction {
    public:
        virtual const char* get_name() const = 0;
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
        std::vector<const char*> get_front_undo_queue_names(unsigned count);

        World& world;
    private:
        void push_undo(std::unique_ptr<WorldUndoAction> undoAction);
        void push_redo(std::unique_ptr<WorldUndoAction> undoAction);
        constexpr static size_t UNDO_QUEUE_LIMIT = 500;

        std::unordered_map<UndoObjectID, NetworkingObjects::NetObjID> undoIDToNetID;
        std::unordered_map<NetworkingObjects::NetObjID, UndoObjectID> netIDToUndoID;
        std::deque<std::unique_ptr<WorldUndoAction>> undoQueue;
        std::deque<std::unique_ptr<WorldUndoAction>> redoQueue;

        UndoObjectID lastUndoObjectID = 0;
};
