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
        size_t queueLimit = 1000;
        std::deque<UndoRedoPair> undoQueue;
        std::deque<UndoRedoPair> redoQueue;
};
