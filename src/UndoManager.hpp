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
#include <functional>
#include <deque>

// NOTE: This undo manager doesn't completely solve all collisions between clients
// Example: If both clients erase the same brush stroke at the same time (before either is notified of the erasure), both can
// undo that step and generate two copies of the same brush stroke. For now, we will deem this to be acceptable since the chances
// of this happening are low, and even if it does happen, it isn't a massive issue (both clients, and the server, will be aware
// of the two brush strokes)
class UndoManager {
    public:
        struct UndoRedoPair {
            std::function<bool()> undo;
            std::function<bool()> redo;
        };

        void push(const UndoRedoPair& undoRedoPair);
        void undo();
        void redo();
        void clear();
        bool can_undo();
        bool can_redo();
    private:
        void push_undo(const UndoRedoPair& undoRedoPair);
        void push_redo(const UndoRedoPair& undoRedoPair);
        size_t queueLimit = 250;
        std::deque<UndoRedoPair> undoQueue;
        std::deque<UndoRedoPair> redoQueue;
};
