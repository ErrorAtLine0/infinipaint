#pragma once
// Thin InfiniPaint adapter. Algorithm source is pinned in deps/pen-stabilizer.
#include "../deps/pen-stabilizer/include/pen_stabilizer/stabilizer.hpp"

namespace PenInput {
using Point = pen_stabilizer::Point;
using Sample = pen_stabilizer::Sample;
struct Settings {
    bool enabled = true;
    double radius = 12, window = .120, cap = 4;
    void validate() {
        if (!std::isfinite(radius)) radius = 12;
        if (!std::isfinite(window)) window = .120;
        if (!std::isfinite(cap)) cap = 4;
        radius = std::clamp(radius, 4.0, 20.0);
        window = std::clamp(window, .040, .200);
        cap = std::clamp(cap, 0.0, 6.0);
    }
};
class Stabilizer : public pen_stabilizer::Stabilizer {
public:
    void reset(Settings settings = {}) {
        settings.validate();
        pen_stabilizer::Stabilizer::reset({settings.enabled, settings.radius, settings.window, settings.cap});
    }
};
}
