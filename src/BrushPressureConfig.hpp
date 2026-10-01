#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <nlohmann/json.hpp>

namespace BrushPressure {
enum class Response { Original, Preserve, Peak };

struct Config {
    bool hasRoundCaps = true;
    float relativeWidth = 15.0f;
    Response pressureResponse = Response::Original;
    bool samplePath() const { return pressureResponse != Response::Original; }
    friend void to_json(nlohmann::json& j, const Config& c) {
        const char* mode = c.pressureResponse == Response::Preserve ? "preserve" :
            c.pressureResponse == Response::Peak ? "peak" : "original";
        j = {{"hasRoundCaps", c.hasRoundCaps}, {"relativeWidth", c.relativeWidth},
            {"pressureResponse", mode},
            // Older fork builds understand this key, but not peak mode.
            {"preservePenPressure", c.pressureResponse == Response::Preserve}};
    }
    friend void from_json(const nlohmann::json& j, Config& c) {
        c = Config{};
        if (j.contains("hasRoundCaps") && j["hasRoundCaps"].is_boolean())
            c.hasRoundCaps = j["hasRoundCaps"].get<bool>();
        if (j.contains("relativeWidth") && j["relativeWidth"].is_number()) {
            const float width = j["relativeWidth"].get<float>();
            if (std::isfinite(width) && width >= 0) c.relativeWidth = width;
        }
        if (j.contains("pressureResponse")) {
            if (j["pressureResponse"] == "preserve") c.pressureResponse = Response::Preserve;
            else if (j["pressureResponse"] == "peak") c.pressureResponse = Response::Peak;
            // Unknown new mode safely falls back to Original, not a legacy key.
        } else if (j.contains("preservePenPressure") && j["preservePenPressure"].is_boolean() &&
                   j["preservePenPressure"].get<bool>()) c.pressureResponse = Response::Preserve;
    }
};
}
