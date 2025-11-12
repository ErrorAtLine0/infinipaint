#include "UndoManager.hpp"
#include <iostream>

bool UndoManager::can_undo() {
    return !undoQueue.empty();
}

bool UndoManager::can_redo() {
    return !redoQueue.empty();
}

void UndoManager::push(const UndoRedoPair& undoRedoPair) {
    push_undo(undoRedoPair);
    redoQueue.clear();
}

void UndoManager::push_undo(const UndoRedoPair& undoRedoPair) {
    if(undoQueue.size() == queueLimit)
        undoQueue.pop_front();
    undoQueue.emplace_back(undoRedoPair);
}

void UndoManager::push_redo(const UndoRedoPair& undoRedoPair) {
    if(redoQueue.size() == queueLimit)
        redoQueue.pop_front();
    redoQueue.emplace_back(undoRedoPair);
}

void UndoManager::undo() {
    if(undoQueue.empty())
        return;
    if(!undoQueue.back().undo()) {
        clear();
    }
    else {
        push_redo(undoQueue.back());
        undoQueue.pop_back();
    }
}

void UndoManager::redo() {
    if(redoQueue.empty())
        return;
    if(!redoQueue.back().redo()) {
        clear();
    }
    else {
        push_undo(redoQueue.back());
        redoQueue.pop_back();
    }
}

void UndoManager::clear() {
    redoQueue.clear();
    undoQueue.clear();
}
