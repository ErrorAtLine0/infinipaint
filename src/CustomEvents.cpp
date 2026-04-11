#include "CustomEvents.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <queue>

namespace CustomEvents {

std::queue<std::any> eventDataQueue;

bool alreadyInitialized = false;

void init() {
    if(!alreadyInitialized) {
        PasteEvent::EVENT_NUM = SDL_RegisterEvents(1);
        OpenInfiniPaintFileEvent::EVENT_NUM = SDL_RegisterEvents(1);
        AddFileToCanvasEvent::EVENT_NUM = SDL_RegisterEvents(1);
        alreadyInitialized = true;
    }
}

void pop_event() {
    eventDataQueue.pop();
}

uint32_t PasteEvent::EVENT_NUM;
uint32_t OpenInfiniPaintFileEvent::EVENT_NUM;
uint32_t AddFileToCanvasEvent::EVENT_NUM;

}
