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

#include "CustomEvents.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <queue>

namespace CustomEvents {

    InputTextBoxID text_box_get_new_id() {
        static InputTextBoxID nextID = 0;
        ++nextID;
        return nextID;
    }

std::queue<std::shared_ptr<void>> eventDataQueue;
std::mutex eventDataQueueMutex;

bool alreadyInitialized = false;

void init() {
    if(!alreadyInitialized) {
        PasteEvent::EVENT_NUM = SDL_RegisterEvents(1);
        OpenInfiniPaintFileEvent::EVENT_NUM = SDL_RegisterEvents(1);
        AddFileToCanvasEvent::EVENT_NUM = SDL_RegisterEvents(1);
        RefreshTextBoxInputEvent::EVENT_NUM = SDL_RegisterEvents(1);
        AndroidTextBoxInputEvent::EVENT_NUM = SDL_RegisterEvents(1);
        MobileImportCanvasEvent::EVENT_NUM = SDL_RegisterEvents(1);
        alreadyInitialized = true;
    }
}

uint32_t PasteEvent::EVENT_NUM;
uint32_t OpenInfiniPaintFileEvent::EVENT_NUM;
uint32_t AddFileToCanvasEvent::EVENT_NUM;
uint32_t RefreshTextBoxInputEvent::EVENT_NUM;
uint32_t AndroidTextBoxInputEvent::EVENT_NUM;
uint32_t MobileImportCanvasEvent::EVENT_NUM;

}
