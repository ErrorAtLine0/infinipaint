#pragma once
#include "ResourceDisplay.hpp"
#include <include/core/SkImage.h>
#include <vector>
#include <thread>
#include <include/codec/SkCodec.h>

class ImageResourceDisplay : public ResourceDisplay {
    public:
        virtual void update(World& w) override;
        virtual bool load(ResourceManager& rMan, const std::string& fileName, const std::string& fileData) override;
        virtual bool update_draw() const override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData, const SkRect& imRect) override;
        virtual Vector2f get_dimensions() const override;
        virtual float get_dimension_scale() const override;
        virtual Type get_type() const override;
        virtual ~ImageResourceDisplay() override;
    private:
        struct FrameData {
            sk_sp<SkImage> data;
            float duration;
            std::unordered_map<unsigned, sk_sp<SkImage>> mipmapLevels;
        };
        std::vector<FrameData> frames;

        SkImageInfo imageInfo;

        void load_thread();

        int totalFramesToLoad;
        std::atomic<int> loadedFrames = 0;
        sk_sp<SkData> imageRawData;
        std::unique_ptr<SkCodec> codec;
        std::atomic<bool> shutdownLoadThread = false;
        std::unique_ptr<std::thread> loadThread;

        float currentTime = 0.0f;
        int frameIndex = 0;
        bool mustUpdateDraw = false;
        bool loadedFirstFrame = false;
};
