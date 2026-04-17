#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <optional>
#include <queue>
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <mutex>
#include "RichText/TextBox.hpp"

namespace CustomEvents {
    void init();

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
        std::optional<std::filesystem::path> filePathSource;
        std::string netSource;
        std::string serverLocalID;
        std::string_view fileDataBuffer;
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
}
