#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

namespace BrushPressure {
// Attribute policies never modify the immutable source or corrected positions.
class SampleWidths {
public:
    void reset(bool uniformPeak, float firstWidth, bool smooth=false, float factor=.707f) {
        peak=uniformPeak; maximum=firstWidth; smoothing=smooth && !peak;
        propagation=std::isfinite(factor) ? std::clamp(factor,0.0f,1.0f) : .707f;
        widths.clear(); if(smoothing) widths.push_back(firstWidth);
    }
    bool append(float width) {
        const float previous=maximum;
        maximum=std::max(maximum,width);
        if(smoothing) {
            widths.push_back(width);
            widths.back()=std::max(widths.back(),widths[widths.size()-2]*propagation);
            for(size_t i=widths.size()-1;i>0;--i) {
                const float raised=widths[i]*propagation;
                if(raised<=widths[i-1]) break;
                widths[i-1]=raised;
            }
        }
        return smoothing || (peak && maximum>previous);
    }
    float output(float sourceWidth, size_t index=0) const {
        return peak ? maximum : smoothing ? widths.at(index) : sourceWidth;
    }
private:
    bool peak=false, smoothing=false;
    float maximum=0, propagation=.707f;
    std::vector<float> widths;
};
}
