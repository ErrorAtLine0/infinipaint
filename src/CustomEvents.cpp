#include "CustomEvents.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <queue>

namespace CustomEvents {

bool alreadyInitialized = false;

void init() {
    if(!alreadyInitialized) {
        PASTE_EVENT = SDL_RegisterEvents(1);
        OPEN_INFINIPAINT_FILE_EVENT = SDL_RegisterEvents(1);
        alreadyInitialized = true;
    }
}


// Paste event

uint32_t PASTE_EVENT;
std::queue<PasteEventData> pasteEventData;

bool emit_paste_event(const PasteEventData& pasteData) {
    SDL_Event event;
    SDL_zero(event);
    event.type = PASTE_EVENT;
    if(SDL_PushEvent(&event)) {
        pasteEventData.push(pasteData);
        return true;
    }
    return false;
}

const PasteEventData& get_paste_event_data() {
    return pasteEventData.front();
}

void pop_paste_event_data() {
    pasteEventData.pop();
}


// Open InfiniPaint file event

uint32_t OPEN_INFINIPAINT_FILE_EVENT;
std::queue<OpenInfiniPaintFileEventData> openInfiniPaintFileEventData;

bool emit_open_infinipaint_file_event(const OpenInfiniPaintFileEventData& openData) {
    SDL_Event event;
    SDL_zero(event);
    event.type = OPEN_INFINIPAINT_FILE_EVENT;
    if(SDL_PushEvent(&event)) {
        openInfiniPaintFileEventData.push(openData);
        return true;
    }
    return false;
}

const OpenInfiniPaintFileEventData& get_open_infinipaint_file_event_data() {
    return openInfiniPaintFileEventData.front();
}

void pop_open_infinipaint_file_event_data() {
    openInfiniPaintFileEventData.pop();
}

}
