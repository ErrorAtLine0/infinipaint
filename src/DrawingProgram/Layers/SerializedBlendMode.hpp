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
    {SerializedBlendMode::SRC, "Src"},
    {SerializedBlendMode::DST, "Dst"},
    {SerializedBlendMode::SRC_OVER, "Src Over"},
    {SerializedBlendMode::DST_OVER, "Dst Over"},
    {SerializedBlendMode::SRC_IN, "Src In"},
    {SerializedBlendMode::DST_IN, "Dst In"},
    {SerializedBlendMode::SRC_OUT, "Src Out"},
    {SerializedBlendMode::DST_OUT, "Dst Out"},
    {SerializedBlendMode::SRC_A_TOP, "Src A Top"},
    {SerializedBlendMode::DST_A_TOP, "Dst A Top"},
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
const std::vector<std::string>& get_blend_mode_name_list();
