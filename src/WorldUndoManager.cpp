#include "WorldUndoManager.hpp"

void WorldUndoAction::scale_up(const WorldScalar& scaleAmount) {}
WorldUndoAction::~WorldUndoAction() {}

WorldUndoManager::WorldUndoManager(World& initWorld):
    world(initWorld)
{}

bool WorldUndoManager::can_undo() {
    return !undoQueue.empty();
}

bool WorldUndoManager::can_redo() {
    return !redoQueue.empty();
}

void WorldUndoManager::push(std::unique_ptr<WorldUndoAction> undoAction) {
    push_undo(std::move(undoAction));
    redoQueue.clear();
}

void WorldUndoManager::push_undo(std::unique_ptr<WorldUndoAction> undoAction) {
    if(undoQueue.size() == UNDO_QUEUE_LIMIT)
        undoQueue.pop_front();
    undoQueue.emplace_back(std::move(undoAction));
}

void WorldUndoManager::push_redo(std::unique_ptr<WorldUndoAction> undoAction) {
    if(redoQueue.size() == UNDO_QUEUE_LIMIT)
        redoQueue.pop_front();
    redoQueue.emplace_back(std::move(undoAction));
}

void WorldUndoManager::undo() {
    if(undoQueue.empty())
        return;
    if(!undoQueue.back()->undo(*this))
        clear();
    else {
        push_redo(std::move(undoQueue.back()));
        undoQueue.pop_back();
    }
}

void WorldUndoManager::redo() {
    if(redoQueue.empty())
        return;
    if(!redoQueue.back()->redo(*this))
        clear();
    else {
        push_undo(std::move(redoQueue.back()));
        redoQueue.pop_back();
    }
}

void WorldUndoManager::clear() {
    redoQueue.clear();
    undoQueue.clear();
}

void WorldUndoManager::scale_up(const WorldScalar& scaleAmount) {
    for(auto& u : undoQueue)
        u->scale_up(scaleAmount);
    for(auto& r : redoQueue)
        r->scale_up(scaleAmount);
}

void WorldUndoManager::reassign_netid(const NetworkingObjects::NetObjID& oldNetObjID, const NetworkingObjects::NetObjID& newNetObjID) {
    auto netIDtoUndoIDIterator = netIDToUndoID.find(oldNetObjID);
    if(netIDtoUndoIDIterator != netIDToUndoID.end()) {
        UndoObjectID undoID = netIDtoUndoIDIterator->second;
        netIDToUndoID.erase(netIDtoUndoIDIterator);
        netIDToUndoID[newNetObjID] = undoID;
        undoIDToNetID[undoID] = newNetObjID;
    }
}

void WorldUndoManager::remove_by_netid(const NetworkingObjects::NetObjID& netObjID) {
    auto netIDtoUndoIDIterator = netIDToUndoID.find(netObjID);
    if(netIDtoUndoIDIterator != netIDToUndoID.end()) {
        undoIDToNetID.erase(netIDtoUndoIDIterator->second);
        netIDToUndoID.erase(netIDtoUndoIDIterator);
    }
}

void WorldUndoManager::remove_by_undoid(UndoObjectID undoID) {
    auto undoIDtoNetIDIterator = undoIDToNetID.find(undoID);
    if(undoIDtoNetIDIterator != undoIDToNetID.end()) {
        netIDToUndoID.erase(undoIDtoNetIDIterator->second);
        undoIDToNetID.erase(undoID);
    }
}

WorldUndoManager::UndoObjectID WorldUndoManager::get_undoid_from_netid(const NetworkingObjects::NetObjID& netObjID) {
    auto it = netIDToUndoID.find(netObjID);
    if(it == netIDToUndoID.end()) {
        ++lastUndoObjectID;
        undoIDToNetID[lastUndoObjectID] = netObjID;
        netIDToUndoID[netObjID] = lastUndoObjectID;
        return lastUndoObjectID;
    }
    return it->second;
}

void WorldUndoManager::register_new_netid_to_existing_undoid(UndoObjectID existingUndoID, const NetworkingObjects::NetObjID& netObjID) {
    auto undoIDToNetIDIterator = undoIDToNetID.find(existingUndoID);
    if(undoIDToNetIDIterator != undoIDToNetID.end()) {
        netIDToUndoID.erase(undoIDToNetIDIterator->second);
        undoIDToNetIDIterator->second = netObjID;
        netIDToUndoID[netObjID] = existingUndoID;
    }
    else {
        netIDToUndoID[netObjID] = existingUndoID;
        undoIDToNetID[existingUndoID] = netObjID;
    }
}

std::optional<NetworkingObjects::NetObjID> WorldUndoManager::get_netid_from_undoid(UndoObjectID undoID) {
    auto it = undoIDToNetID.find(undoID);
    if(it == undoIDToNetID.end())
        return std::nullopt;
    return it->second;
}

bool WorldUndoManager::fill_netid_list_from_undoid_list(std::vector<NetworkingObjects::NetObjID>& netIDList, const std::vector<UndoObjectID>& undoIDList) {
    for(UndoObjectID undoID : undoIDList) {
        auto netIDOpt = get_netid_from_undoid(undoID);
        if(!netIDOpt.has_value())
            return false;
        netIDList.emplace_back(netIDOpt.value());
    }
    return true;
}

std::vector<std::string> WorldUndoManager::get_front_undo_queue_names(unsigned count) {
    std::vector<std::string> toRet;
    for(auto& u : undoQueue | std::views::reverse) {
        toRet.emplace_back(u->get_name());
        if(toRet.size() == count)
            return toRet;
    }
    return toRet;
}
