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

#include "UndoManager.hpp"
#include <iostream>

// NOTE: It might be possible to track net objects through their raw pointer values instead of NetObjIDs, as the NetObjID is just reassigned when moved, and it isn't reallocated.

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
