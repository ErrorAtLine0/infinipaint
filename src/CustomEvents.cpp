#include "CustomEvents.hpp"
#include <SDL3/SDL.h>
#include <queue>

namespace CustomEvents {

bool alreadyInitialized = false;

uint32_t PASTE_EVENT;
std::queue<PasteEventData> pasteEventData;

void init() {
    if(!alreadyInitialized) {
        PASTE_EVENT = SDL_RegisterEvents(1);
        alreadyInitialized = true;
    }
}

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

}
