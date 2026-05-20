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
#include <cstdint>
#include <memory>
#include <string>
#include <optional>
#include <queue>
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <mutex>
#include <Helpers/SCollision.hpp>
#include "RichText/TextBox.hpp"

namespace CustomEvents {
    void init();

    typedef int64_t InputTextBoxID;
    InputTextBoxID text_box_get_new_id();

    extern std::queue<std::shared_ptr<void>> eventDataQueue;
    extern std::mutex eventDataQueueMutex;

    template <typename T> bool emit_event(const T& data) {
        std::scoped_lock l{eventDataQueueMutex};
        SDL_Event event;
        SDL_zero(event);
        event.type = T::EVENT_NUM;
        if(SDL_PushEvent(&event)) {
            eventDataQueue.push(std::make_shared<T>(data));
            return true;
        }
        return false;
    }

    template <typename T> std::shared_ptr<T> get_event() {
        std::scoped_lock l{eventDataQueueMutex};
        std::shared_ptr<T> toRet = std::static_pointer_cast<T>(eventDataQueue.front());
        eventDataQueue.pop();
        return toRet;
    }

    struct PasteEvent {
        static uint32_t EVENT_NUM;
        enum class DataType {
            TEXT,
            IMAGE
        } type;
        std::string data;
        std::optional<RichText::TextData> richText;
        std::optional<Vector2f> mousePos;
    };

    struct OpenInfiniPaintFileEvent {
        static uint32_t EVENT_NUM;
        bool isClient;
        bool saveThumbnail = false;
        std::optional<std::filesystem::path> filePathEmptyAutoSaveDir;
        std::optional<std::filesystem::path> filePathSource;
        std::string netSource;
        std::string serverLocalID;
        std::string fileDataBuffer;
    };

    struct AddFileToCanvasEvent {
        static uint32_t EVENT_NUM;
        enum class Type {
            BUFFER,
            PATH
        } type;
        std::string name;
        std::string buffer;
        std::filesystem::path filePath;
        Vector2f pos;
    };

    struct RefreshTextBoxInputEvent {
        static uint32_t EVENT_NUM;
    };

    struct AndroidTextBoxInputEvent {
        static uint32_t EVENT_NUM;
        InputTextBoxID textboxID;
        enum class CommandType {
            COMMIT_ALL
        } command;
        std::string strData;
        Vector3i intData;
    };

    struct AndroidInsetsChangedEvent {
        static uint32_t EVENT_NUM;
        SCollision::AABB<float> safeWindowRect;
    };
}
