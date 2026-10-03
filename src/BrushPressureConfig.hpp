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
    bool correctionIndependent = false; // Migrated once after both configs load.
    bool samplePath(bool correction=false) const { return correction || pressureResponse != Response::Original; }
    void migrateCorrection(bool& correction) {
        if (!correctionIndependent && pressureResponse == Response::Original) correction=false;
        correctionIndependent=true;
    }
    friend void to_json(nlohmann::json& j, const Config& c) {
        const char* mode = c.pressureResponse == Response::Preserve ? "preserve" :
            c.pressureResponse == Response::Peak ? "peak" : "original";
        j = {{"hasRoundCaps", c.hasRoundCaps}, {"relativeWidth", c.relativeWidth},
            {"pressureResponse", mode}, {"correctionIndependent",c.correctionIndependent},
            // Older fork builds understand this key, but not peak mode.
            {"preservePenPressure", c.pressureResponse == Response::Preserve}};
    }
    friend void from_json(const nlohmann::json& j, Config& c) {
        c = Config{};
        if (j.contains("correctionIndependent") && j["correctionIndependent"].is_boolean())
            c.correctionIndependent=j["correctionIndependent"].get<bool>();
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
