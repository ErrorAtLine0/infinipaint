#pragma once
#include <include/core/SkBlendMode.h>
#include <nlohmann/json.hpp>

enum class SerializedBlendMode : uint8_t {
    BLEND_CLEAR = 0,
    BLEND_SRC,
    BLEND_DST,
    BLEND_SRC_OVER,
    BLEND_DST_OVER,
    BLEND_SRC_IN,
    BLEND_DST_IN,
    BLEND_SRC_OUT,
    BLEND_DST_OUT,
    BLEND_SRC_A_TOP,
    BLEND_DST_A_TOP,
    BLEND_XOR,
    BLEND_PLUS,
    BLEND_MODULATE,
    BLEND_SCREEN,
    BLEND_OVERLAY,
    BLEND_DARKEN,
    BLEND_LIGHTEN,
    BLEND_COLOR_DODGE,
    BLEND_COLOR_BURN,
    BLEND_HARD_LIGHT,
    BLEND_SOFT_LIGHT,
    BLEND_DIFFERENCE,
    BLEND_EXCLUSION,
    BLEND_MULTIPLY,
    BLEND_HUE,
    BLEND_SATURATION,
    BLEND_COLOR,
    BLEND_LUMINOSITY,

    BLEND_LAST_MODE
};

NLOHMANN_JSON_SERIALIZE_ENUM(SerializedBlendMode, {
    {SerializedBlendMode::BLEND_CLEAR, "Clear"},
    {SerializedBlendMode::BLEND_SRC, "Source"},
    {SerializedBlendMode::BLEND_DST, "Destination"},
    {SerializedBlendMode::BLEND_SRC_OVER, "Source Over"},
    {SerializedBlendMode::BLEND_DST_OVER, "Destination Over"},
    {SerializedBlendMode::BLEND_SRC_IN, "Source In"},
    {SerializedBlendMode::BLEND_DST_IN, "Destination In"},
    {SerializedBlendMode::BLEND_SRC_OUT, "Source Out"},
    {SerializedBlendMode::BLEND_DST_OUT, "Destination Out"},
    {SerializedBlendMode::BLEND_SRC_A_TOP, "Source Alpha Top"},
    {SerializedBlendMode::BLEND_DST_A_TOP, "Destination Alpha Top"},
    {SerializedBlendMode::BLEND_XOR, "XOR"},
    {SerializedBlendMode::BLEND_PLUS, "Plus"},
    {SerializedBlendMode::BLEND_MODULATE, "Modulate"},
    {SerializedBlendMode::BLEND_SCREEN, "Screen"},
    {SerializedBlendMode::BLEND_OVERLAY, "Overlay"},
    {SerializedBlendMode::BLEND_DARKEN, "Darken"},
    {SerializedBlendMode::BLEND_LIGHTEN, "Lighten"},
    {SerializedBlendMode::BLEND_COLOR_DODGE, "Color Dodge"},
    {SerializedBlendMode::BLEND_COLOR_BURN, "Color Burn"},
    {SerializedBlendMode::BLEND_HARD_LIGHT, "Hard Light"},
    {SerializedBlendMode::BLEND_SOFT_LIGHT, "Soft Light"},
    {SerializedBlendMode::BLEND_DIFFERENCE, "Difference"},
    {SerializedBlendMode::BLEND_EXCLUSION, "Exclusion"},
    {SerializedBlendMode::BLEND_MULTIPLY, "Multiply"},
    {SerializedBlendMode::BLEND_HUE, "Hue"},
    {SerializedBlendMode::BLEND_SATURATION, "Saturation"},
    {SerializedBlendMode::BLEND_COLOR, "Color"},
    {SerializedBlendMode::BLEND_LUMINOSITY, "Luminosity"}
})

SkBlendMode serialized_blend_mode_to_sk_blend_mode(SerializedBlendMode serializedBlendMode);
const std::vector<SerializedBlendMode>& get_blend_mode_useful_list();
const std::vector<std::string>& get_blend_mode_useful_name_list();
