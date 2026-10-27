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
#include <clay.h>
#include <Helpers/MathExtras.hpp>

class CanvasEditorScreen;

class ChatMessageGUI {
    public:
        ChatMessageGUI(CanvasEditorScreen& initScreen);
        void update();
        void layout(const Vector2f& offset, Clay_FloatingAttachPoints attachPoints);
        void set_open(bool newState);
        bool is_open();
    private:
        bool isChatboxOpen = false;
        std::string chatMessageInput;
        CanvasEditorScreen& s;
};
