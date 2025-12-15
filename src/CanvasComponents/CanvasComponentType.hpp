#pragma once
#include <cstdint>

enum class CanvasComponentType : uint8_t {
    BRUSHSTROKE = 0,
    RECTANGLE,
    ELLIPSE,
    TEXTBOX,
    IMAGE
};
