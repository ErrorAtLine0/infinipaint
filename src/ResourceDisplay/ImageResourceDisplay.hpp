#pragma once
#include "ResourceDisplay.hpp"
#include <include/core/SkImage.h>
#include <vector>
#include <thread>
#include <include/codec/SkCodec.h>
#include <queue>

class ImageResourceDisplay : public ResourceDisplay {
    public:
        virtual void update(World& w) override;
        virtual bool load(ResourceManager& rMan, const std::string& fileName, const std::shared_ptr<std::string>& fileData) override;
        virtual bool update_draw() const override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData, const SkRect& imRect) override;
        virtual Vector2f get_dimensions() const override;
        virtual float get_dimension_scale() const override;
        virtual Type get_type() const override;
        virtual void camera_view_update(const CoordSpaceHelper& compCoords, const SCollision::AABB<WorldScalar>& compAABB, const DrawData& drawData, const SkRect& imRect) override;
        virtual void clear_cache() override;
        virtual ~ImageResourceDisplay() override;

        static int IMAGE_LOAD_THREAD_COUNT_MAX;

    private:
        static constexpr int SMALLEST_MIPMAP_RESOLUTION = 128;
        
        struct FrameData {
            // Mipmap level calculation is done using the smaller dimension, not the bigger one, to ensure that the dimensions are never invalid
            std::vector<sk_sp<SkImage>> mipmapLevels;
            sk_sp<SkImage> smallestMipmapLevel; // Smaller dimension of this should be around SMALLEST_MIPMAP_RESOLUTION pixels. Is always allocated and used when cachedMipmapLevel is unavailable or when the image is viewed from far away
                                                // Has auto generated mipmaps
            float duration = -1.0f;
        };

        static std::unordered_map<std::shared_ptr<std::string>, sk_sp<SkImage>> screenshotCache;

        static std::atomic<int> imageLoadThreadCount;

        std::vector<FrameData> frames;

        enum class MipmapLevelStatus {
            UNALLOCATED,
            ALLOCATED_JUST_SET,
            ALLOCATED_WILL_REMOVE
        };

        std::vector<std::atomic<MipmapLevelStatus>> mipmapLevelsStatus;
        std::atomic<bool> smallestMipmapLevelLoaded = false;

        std::shared_ptr<std::string> fileData;
        SkImageInfo imageInfo;

        float currentTime = 0.0f;
        unsigned frameIndex = 0;
        bool mustUpdateDraw = false;

        std::unique_ptr<std::thread> loadThread;
        std::atomic<bool> shutdownLoadThread = false;
        std::atomic<bool> mustUpdateDrawLoadThread = false;

        unsigned get_exact_mipmap_level_for_image_component(const DrawData& drawData, const CoordSpaceHelper& compCoords, const SkRect& imRect);
        unsigned get_exact_mipmap_level_for_dimensions(const Vector2i& dim);
        unsigned get_best_allocated_mipmap_level(unsigned mipmapLevel);
        unsigned get_smallest_mipmap_level() const;
        Vector2i get_mipmap_level_image_dimensions(unsigned mipmapLevel) const;
        unsigned calculate_smallest_mipmap_level();
        void load_thread_func(unsigned mipmapLevel);
        void attempt_load_mipmap_in_separate_thread(unsigned mipmapLevel);
        sk_sp<SkImage> load_frame_with_codec(const std::unique_ptr<SkCodec>& codec, unsigned mipmapLevel, unsigned frameIndexToLoad);
};
