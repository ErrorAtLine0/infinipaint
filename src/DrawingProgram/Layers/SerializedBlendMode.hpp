#pragma once
#include <include/core/SkBlendMode.h>
#include <nlohmann/json.hpp>

enum class SerializedBlendMode : uint8_t {
    CLEAR = 0,
    SRC,
    DST,
    SRC_OVER,
    DST_OVER,
    SRC_IN,
    DST_IN,
    SRC_OUT,
    DST_OUT,
    SRC_A_TOP,
    DST_A_TOP,
    XOR,
    PLUS,
    MODULATE,
    SCREEN,
    OVERLAY,
    DARKEN,
    LIGHTEN,
    COLOR_DODGE,
    COLOR_BURN,
    HARD_LIGHT,
    SOFT_LIGHT,
    DIFFERENCE,
    EXCLUSION,
    MULTIPLY,
    HUE,
    SATURATION,
    COLOR,
    LUMINOSITY,

    LAST_MODE
};

NLOHMANN_JSON_SERIALIZE_ENUM(SerializedBlendMode, {
    {SerializedBlendMode::CLEAR, "Clear"},
    {SerializedBlendMode::SRC, "Source"},
    {SerializedBlendMode::DST, "Destination"},
    {SerializedBlendMode::SRC_OVER, "Source Over"},
    {SerializedBlendMode::DST_OVER, "Destination Over"},
    {SerializedBlendMode::SRC_IN, "Source In"},
    {SerializedBlendMode::DST_IN, "Destination In"},
    {SerializedBlendMode::SRC_OUT, "Source Out"},
    {SerializedBlendMode::DST_OUT, "Destination Out"},
    {SerializedBlendMode::SRC_A_TOP, "Source Alpha Top"},
    {SerializedBlendMode::DST_A_TOP, "Destination Alpha Top"},
    {SerializedBlendMode::XOR, "XOR"},
    {SerializedBlendMode::PLUS, "Plus"},
    {SerializedBlendMode::MODULATE, "Modulate"},
    {SerializedBlendMode::SCREEN, "Screen"},
    {SerializedBlendMode::OVERLAY, "Overlay"},
    {SerializedBlendMode::DARKEN, "Darken"},
    {SerializedBlendMode::LIGHTEN, "Lighten"},
    {SerializedBlendMode::COLOR_DODGE, "Color Dodge"},
    {SerializedBlendMode::COLOR_BURN, "Color Burn"},
    {SerializedBlendMode::HARD_LIGHT, "Hard Light"},
    {SerializedBlendMode::SOFT_LIGHT, "Soft Light"},
    {SerializedBlendMode::DIFFERENCE, "Difference"},
    {SerializedBlendMode::EXCLUSION, "Exclusion"},
    {SerializedBlendMode::MULTIPLY, "Multiply"},
    {SerializedBlendMode::HUE, "Hue"},
    {SerializedBlendMode::SATURATION, "Saturation"},
    {SerializedBlendMode::COLOR, "Color"},
    {SerializedBlendMode::LUMINOSITY, "Luminosity"}
})

SkBlendMode serialized_blend_mode_to_sk_blend_mode(SerializedBlendMode serializedBlendMode);
const std::vector<SerializedBlendMode>& get_blend_mode_useful_list();
const std::vector<std::string>& get_blend_mode_useful_name_list();
