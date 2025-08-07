#pragma once
#include "ResourceDisplay.hpp"
#include <include/core/SkImage.h>
#include <vector>

class ImageResourceDisplay : public ResourceDisplay {
    public:
        virtual void update(World& w) override;
        virtual bool load(ResourceManager& rMan, const std::string& fileName, const std::string& fileData) override;
        virtual bool update_draw() const override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData, const SkRect& imRect) override;
        virtual Vector2f get_dimensions() const override;
        virtual float get_dimension_scale() const override;
    private:
        struct FrameData {
            sk_sp<SkImage> data;
            float duration;
        };
        std::vector<FrameData> frames;
        float currentTime = 0.0f;
        size_t frameIndex = 0;
        bool mustUpdateDraw = false;
};
