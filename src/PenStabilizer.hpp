#pragma once
// Copyright (c) 2026 Pen Trace Lab contributors.
// Adapted under the MIT license; see assets/data/third_party_licenses/PenTraceLab.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include <utility>

// Streaming port of PenTraceLab 0.4.0 localFilter (ef6555a).
// Coordinates are display DIPs; timestamps are report seconds, not frame time.
// Only the bounded live tail changes. No prediction or post-lift catch-up.
namespace PenInput {
struct Point {
    double x = 0, y = 0;
    Point operator+(Point b) const { return {x + b.x, y + b.y}; }
    Point operator-(Point b) const { return {x - b.x, y - b.y}; }
    Point operator*(double s) const { return {x * s, y * s}; }
    double length() const { return std::hypot(x, y); }
    double dot(Point b) const { return x * b.x + y * b.y; }
    bool finite() const { return std::isfinite(x) && std::isfinite(y); }
};
struct Settings {
    bool enabled = true;
    double radius = 12;
    double window = .120;
    double cap = 4;
    void validate() {
        if (!std::isfinite(radius)) radius = 12;
        if (!std::isfinite(window)) window = .120;
        if (!std::isfinite(cap)) cap = 4;
        radius = std::clamp(radius, 4.0, 20.0);
        window = std::clamp(window, .040, .200);
        cap = std::clamp(cap, 0.0, 6.0);
    }
};
struct Sample {
    Point position;
    double time = 0;
    float width = 1;
};

// Source samples (including width) remain immutable for the lifetime of a
// stroke. The existing document format stores the resulting mesh, not these
// reports. Changing this format is deliberately outside this filter port.
class Stabilizer {
public:
    void reset(Settings settings = {}) {
        settings.validate();
        options = settings;
        source.clear(); output.clear(); arc.clear(); area.clear();
        runStart = mutableBegin = changed = 0;
    }
    bool append(Sample sample, bool continuous = true) {
        changed = output.size();
        if (!sample.position.finite() || !std::isfinite(sample.time) ||
            !std::isfinite(sample.width) || sample.width < 0) return false;
        const auto n = source.size();
        const double dt = n ? sample.time - source.back().time : 0;
        const bool boundary = !n || !continuous || dt <= 0 || dt > .050;
        if (boundary) runStart = mutableBegin = n;
        source.push_back(sample);
        output.push_back(sample.position);
        const double ds = boundary ? 0 : (sample.position - source[n-1].position).length();
        arc.push_back(boundary ? 0 : arc.back() + ds);
        area.push_back(boundary ? Point{} :
            area.back() + (sample.position + source[n-1].position) * (ds * .5));
        if (!options.enabled || options.cap == 0) { mutableBegin = n+1; return true; }
        changed = std::max(runStart, mutableBegin);
        // Also recalculate points that just left the live window, once, before
        // freezing them. This makes batch boundaries irrelevant to the result.
        for (std::size_t k = changed; k <= n; ++k) output[k] = filtered(k);
        while (mutableBegin <= n && sample.time - source[mutableBegin].time >= options.window)
            ++mutableBegin;
        return true;
    }
    const std::vector<Sample>& samples() const { return source; }
    const std::vector<Point>& positions() const { return output; }
    std::size_t changedBegin() const { return changed; }
    const Settings& settings() const { return options; }

private:
    std::pair<Point, Point> at(double distance) const {
        auto hi = static_cast<std::size_t>(std::upper_bound(arc.begin()+runStart, arc.end(), distance)-arc.begin());
        hi = std::clamp(hi, runStart+1, source.size()-1);
        const double ds = arc[hi]-arc[hi-1];
        const double fraction = ds>0 ? (distance-arc[hi-1])/ds : 0;
        const auto p = source[hi-1].position + (source[hi].position-source[hi-1].position)*fraction;
        return {p, area[hi-1]+(source[hi-1].position+p)*((distance-arc[hi-1])*.5)};
    }
    Point filtered(std::size_t k) const {
        const auto p = source[k].position;
        if (k == runStart || k+1 == source.size()) return p;
        const double t = source[k].time;
        const auto begin = source.begin()+runStart;
        const auto left = static_cast<std::size_t>(std::lower_bound(begin, source.end(), t-options.window,
            [](const Sample& s, double time) { return s.time < time; })-source.begin());
        const auto right = static_cast<std::size_t>(std::upper_bound(begin, source.end(), t+options.window,
            [](double time, const Sample& s) { return time < s.time; })-source.begin()-1);
        const double r = std::min({options.radius, arc[k]-arc[left], arc[right]-arc[k]});
        if (r < 1e-6) return p;
        const auto [a, ia] = at(arc[k]-r);
        const auto [b, ib] = at(arc[k]+r);
        const auto u=p-a, v=b-p, tangent=b-a;
        const double lu=u.length(), lv=v.length(), lt=tangent.length();
        if (lu<1e-6 || lv<1e-6 || lt<1e-6) return p;
        const double cosine = std::clamp(u.dot(v)/(lu*lv), -1.0, 1.0);
        const double corner = std::clamp((cosine-.5)/(.8660254037844386-.5), 0.0, 1.0);
        const double edge = std::min({1.0, (t-source[runStart].time)/options.window,
            (source.back().time-t)/options.window});
        const double gain = corner*edge*edge*(3-2*edge);
        const Point normal{-tangent.y/lt, tangent.x/lt};
        const auto delta=(ib-ia)*(1/(2*r))-p;
        return p+normal*(std::clamp(delta.dot(normal), -options.cap, options.cap)*gain);
    }
    Settings options;
    std::vector<Sample> source;
    std::vector<Point> output, area;
    std::vector<double> arc;
    std::size_t runStart = 0, mutableBegin = 0, changed = 0;
};
}
