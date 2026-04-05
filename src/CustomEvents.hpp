#pragma once
#include <cstdint>
#include <string>
#include <optional>
#include "RichText/TextBox.hpp"

namespace CustomEvents {
    void init();

    // Paste event
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


    // Open InfiniPaint file event

    struct OpenInfiniPaintFileEventData {
        bool isClient;
        std::optional<std::filesystem::path> filePathSource;
        std::string netSource;
        std::string serverLocalID;
        std::string_view fileDataBuffer;
    };

    bool emit_open_infinipaint_file_event(const OpenInfiniPaintFileEventData& openData);
    const OpenInfiniPaintFileEventData& get_open_infinipaint_file_event_data();
    void pop_open_infinipaint_file_event_data();

    extern uint32_t OPEN_INFINIPAINT_FILE_EVENT;
}
