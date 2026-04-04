#pragma once
#include <cstdint>
#include <string>
#include <optional>
#include "RichText/TextBox.hpp"

namespace CustomEvents {
    void init();

    enum class PasteEventDataType {
        TEXT,
        IMAGE
    };
    struct PasteEventData {
        PasteEventDataType type;
        std::string data;
        std::optional<RichText::TextData> richText;
        std::optional<Vector2f> mousePos;
    };

    bool emit_paste_event(const PasteEventData& pasteData);
    const PasteEventData& get_paste_event_data();
    void pop_paste_event_data();

    extern uint32_t PASTE_EVENT;
}
