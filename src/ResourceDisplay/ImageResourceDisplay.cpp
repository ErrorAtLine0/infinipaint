#include "ImageResourceDisplay.hpp"
#include <chrono>
#include <include/codec/SkPngDecoder.h>
#include <include/codec/SkJpegDecoder.h>
#include <include/codec/SkBmpDecoder.h>
#include <include/codec/SkIcoDecoder.h>
#include <include/codec/SkGifDecoder.h>
//#include <include/codec/SkRawDecoder.h>
#include <include/codec/SkWbmpDecoder.h>
#include <include/codec/SkWebpDecoder.h>
#include <include/core/SkCanvas.h>
#include "../MainProgram.hpp"
#include <Helpers/Logger.hpp>
#include <include/core/SkImageInfo.h>
#include <include/core/SkSamplingOptions.h>
#include <bit>
#include <limits>

#ifdef USE_SKIA_BACKEND_GRAPHITE
    #include <include/gpu/graphite/Surface.h>
#elif USE_SKIA_BACKEND_GANESH
    #include <include/gpu/ganesh/GrDirectContext.h>
    #include <include/gpu/ganesh/SkSurfaceGanesh.h>
#endif

std::array<const SkCodecs::Decoder, 7> decoders2 = {
    SkBmpDecoder::Decoder(),
    SkGifDecoder::Decoder(),
    SkIcoDecoder::Decoder(),
    SkJpegDecoder::Decoder(),
    SkPngDecoder::Decoder(),
//    SkRawDecoder::Decoder(),
    SkWbmpDecoder::Decoder(),
    SkWebpDecoder::Decoder()
};

std::unordered_map<std::shared_ptr<std::string>, sk_sp<SkImage>> ImageResourceDisplay::screenshotCache;
std::atomic<int> ImageResourceDisplay::imageLoadThreadCount = 0;

#ifdef __EMSCRIPTEN__
    int ImageResourceDisplay::IMAGE_LOAD_THREAD_COUNT_MAX = 1;
#else
    int ImageResourceDisplay::IMAGE_LOAD_THREAD_COUNT_MAX = 4;
#endif

bool ImageResourceDisplay::update_draw() const {
    return mustUpdateDraw;
}

void ImageResourceDisplay::update(World& w) {
    screenshotCache.clear();

    for(size_t mipmapLevel = 0; mipmapLevel < mipmapLevelsStatus.size(); mipmapLevel++) {
        if(mipmapLevelsStatus[mipmapLevel] == MipmapLevelStatus::ALLOCATED_JUST_SET)
            mipmapLevelsStatus[mipmapLevel] = MipmapLevelStatus::ALLOCATED_WILL_REMOVE;
        else if(mipmapLevelsStatus[mipmapLevel] == MipmapLevelStatus::ALLOCATED_WILL_REMOVE) {
            mipmapLevelsStatus[mipmapLevel] = MipmapLevelStatus::UNALLOCATED;
            for(auto& frame : frames)
                frame.mipmapLevels[mipmapLevel] = nullptr;
        }
    }

    if(mustUpdateDrawLoadThread) {
        mustUpdateDraw = true;
        mustUpdateDrawLoadThread = false;
    }
    else
        mustUpdateDraw = false;

    if(frames.size() == 1)
        return;

    currentTime += w.main.deltaTime;
    for(;;) {
        if(currentTime >= frames[frameIndex].duration) {
            currentTime -= frames[frameIndex].duration;
            frameIndex++;
            mustUpdateDraw = true;
            if(frameIndex >= frames.size())
                frameIndex = 0;
        }
        else
            return;
        if(frames[frameIndex].duration <= 0.0f)
            return;
    }
}

sk_sp<SkImage> ImageResourceDisplay::load_frame_with_codec(const std::unique_ptr<SkCodec>& codec, unsigned width, unsigned height, unsigned frameIndexToLoad) {
    SkCodec::Options imageOptions;
    imageOptions.fFrameIndex = frameIndexToLoad;
    auto decodeResult = codec->getImage(imageInfo, &imageOptions);
    if(std::get<1>(decodeResult) != SkCodec::Result::kSuccess)
        throw std::runtime_error("Could not decode image, got error code " + std::to_string(std::get<1>(decodeResult)) + ". Image decoded with dimensions " + std::to_string(imageInfo.width()) + " " + std::to_string(imageInfo.height()));
    sk_sp<SkImage> ogImage = std::get<0>(decodeResult);
    SkImageInfo scaledImageInfo = imageInfo.makeDimensions(SkISize(width, height));
    auto scaledImage = ogImage->makeScaled(scaledImageInfo, {SkFilterMode::kLinear, SkMipmapMode::kNone});
    if(!scaledImage)
        throw std::runtime_error("Could not scale image.");
    return scaledImage;
}

void ImageResourceDisplay::camera_view_update(const CoordSpaceHelper& compCoords, const SCollision::AABB<WorldScalar>& compAABB, const DrawData& drawData, const SkRect& imRect) {
    if(!smallestMipmapLevelLoaded)
        attempt_load_mipmap_in_separate_thread(get_smallest_mipmap_level());
    else {
        if(SCollision::collide(compAABB, drawData.cam.viewingAreaGenerousCollider)) {
            unsigned mipmapLevel = get_exact_mipmap_level_for_image_component(drawData, compCoords, imRect);
            if(mipmapLevel != get_smallest_mipmap_level()) {
                if(mipmapLevelsStatus[mipmapLevel] == MipmapLevelStatus::UNALLOCATED) {
                    attempt_load_mipmap_in_separate_thread(mipmapLevel);
                    unsigned nextBestMipmapLevel = get_best_allocated_mipmap_level(mipmapLevel);
                    if(nextBestMipmapLevel != get_smallest_mipmap_level())
                        mipmapLevelsStatus[nextBestMipmapLevel] = MipmapLevelStatus::ALLOCATED_JUST_SET;
                }
                else
                    mipmapLevelsStatus[mipmapLevel] = MipmapLevelStatus::ALLOCATED_JUST_SET;
            }
        }
    }
}

bool ImageResourceDisplay::load(ResourceManager& rMan, const std::string& fileName, const std::shared_ptr<std::string>& fileData) {
    this->fileData = fileData;
    auto codec = SkCodec::MakeFromData(SkData::MakeWithoutCopy(this->fileData->c_str(), this->fileData->size()), decoders2);
    mustUpdateDraw = true;
    mustUpdateDrawLoadThread = true;
    if(codec) {
        auto origin = codec->getOrigin();
        bool rotate = (origin == kLeftTop_SkEncodedOrigin || origin == kRightTop_SkEncodedOrigin || origin == kRightBottom_SkEncodedOrigin || origin == kLeftBottom_SkEncodedOrigin);
        imageInfo = codec->getInfo();

        mipmapLevelsStatus = std::vector<std::atomic<MipmapLevelStatus>>(calculate_smallest_mipmap_level());
        for(auto& mipmapLevelStatus : mipmapLevelsStatus)
            mipmapLevelStatus = MipmapLevelStatus::UNALLOCATED;

        frames.resize(codec->getFrameCount());
        for(size_t i = 0; i < frames.size(); i++) {
            auto& frame = frames[i];
            frame.mipmapLevels.resize(get_smallest_mipmap_level());
            SkCodec::FrameInfo frameInfo;
            if(codec->getFrameInfo(i, &frameInfo))
                frame.duration = static_cast<float>(frameInfo.fDuration) / 1000.0f; // get frametime in seconds
            else
                frame.duration = -1.0f;
        }

        if(rotate) {
            Logger::get().log("INFO", "Image loaded is rotated, so must rotate dimensions");
            imageInfo = imageInfo.makeDimensions(SkISize(imageInfo.height(), imageInfo.width()));
        }
        codec = nullptr;
        attempt_load_mipmap_in_separate_thread(get_smallest_mipmap_level());
        return true;
    }
    return false;
}

void ImageResourceDisplay::attempt_load_mipmap_in_separate_thread(unsigned mipmapLevel) {
    if(imageLoadThreadCount >= IMAGE_LOAD_THREAD_COUNT_MAX)
        return;
    if(!loadThread) {
        shutdownLoadThread = false;
        imageLoadThreadCount++;
        loadThread = std::make_unique<std::thread>(&ImageResourceDisplay::load_thread_func, this, mipmapLevel);
    }
    else if(shutdownLoadThread) {
        loadThread->join();
        shutdownLoadThread = false;
        imageLoadThreadCount++;
        loadThread = std::make_unique<std::thread>(&ImageResourceDisplay::load_thread_func, this, mipmapLevel);
    }
}

void ImageResourceDisplay::load_thread_func(unsigned mipmapLevel) {
    auto codec = SkCodec::MakeFromData(SkData::MakeWithoutCopy(this->fileData->c_str(), this->fileData->size()), decoders2);
    Vector2i imDim = get_mipmap_level_image_dimensions(mipmapLevel);
    for(size_t i = 0; i < frames.size(); i++) {
        if(shutdownLoadThread)
            return;
        auto& frame = frames[i];
        auto& mipmapImage = (mipmapLevel == get_smallest_mipmap_level()) ? frame.smallestMipmapLevel : frame.mipmapLevels[mipmapLevel];
        mipmapImage = load_frame_with_codec(codec, imDim.x(), imDim.y(), i);
    }
    if(mipmapLevel == get_smallest_mipmap_level())
        smallestMipmapLevelLoaded = true;
    else
        mipmapLevelsStatus[mipmapLevel] = MipmapLevelStatus::ALLOCATED_JUST_SET;
    mustUpdateDrawLoadThread = true;
    imageLoadThreadCount--;
    shutdownLoadThread = true;
}

void ImageResourceDisplay::draw(SkCanvas* canvas, const DrawData& drawData, const SkRect& imRect) {
    if(!smallestMipmapLevelLoaded)
        canvas->drawRect(imRect, SkPaint({0.5f, 0.5f, 0.5f, 0.5f}));
    else {
        SkRect imRectPixelSize = canvas->getLocalToDeviceAs3x3().mapRect(imRect);
        unsigned mipmapLevel = get_exact_mipmap_level_for_dimensions({imRectPixelSize.width(), imRectPixelSize.height()});
        auto& frame = frames[frameIndex];
        unsigned closestMipmapLevel = get_best_allocated_mipmap_level(mipmapLevel);
        if(drawData.main->takingScreenshot) {
            if(closestMipmapLevel == mipmapLevel) {
                if(closestMipmapLevel == get_smallest_mipmap_level())
                    canvas->drawImageRect(frame.smallestMipmapLevel, imRect, {SkFilterMode::kLinear, SkMipmapMode::kNone});
                else
                    canvas->drawImageRect(frame.mipmapLevels[closestMipmapLevel], imRect, {SkFilterMode::kLinear, SkMipmapMode::kNone});
            }
            else {
                if(screenshotCache.contains(fileData))
                    canvas->drawImageRect(screenshotCache[fileData], imRect, {SkFilterMode::kLinear, SkMipmapMode::kNone});
                else {
                    auto codec = SkCodec::MakeFromData(SkData::MakeWithoutCopy(this->fileData->c_str(), this->fileData->size()), decoders2);
                    screenshotCache[fileData] = load_frame_with_codec(codec, imageInfo.width(), imageInfo.height(), frameIndex);
                    canvas->drawImageRect(screenshotCache[fileData], imRect, {SkFilterMode::kLinear, SkMipmapMode::kNone});
                }
            }
        }
        else {
            if(closestMipmapLevel == get_smallest_mipmap_level())
                canvas->drawImageRect(frame.smallestMipmapLevel, imRect, {SkFilterMode::kLinear, SkMipmapMode::kNone});
            else
                canvas->drawImageRect(frame.mipmapLevels[closestMipmapLevel], imRect, {SkFilterMode::kLinear, SkMipmapMode::kNone});
        }
    }
}

unsigned ImageResourceDisplay::get_exact_mipmap_level_for_image_component(const DrawData& drawData, const CoordSpaceHelper& compCoords, const SkRect& imRect) {
    unsigned minimumRectDim;
    if(imRect.width() < imRect.height())
        minimumRectDim = drawData.cam.c.scalar_to_space(compCoords.scalar_from_space(imRect.width()));
    else
        minimumRectDim = drawData.cam.c.scalar_to_space(compCoords.scalar_from_space(imRect.height()));
    unsigned minimumImageDim = std::min(imageInfo.width(), imageInfo.height());
    unsigned imDim = std::max<unsigned>(minimumRectDim, SMALLEST_MIPMAP_RESOLUTION);
    unsigned divisionLevel = minimumImageDim / imDim;
    return (divisionLevel <= 1) ? 0 : std::bit_width(divisionLevel - 1);
}

unsigned ImageResourceDisplay::get_exact_mipmap_level_for_dimensions(const Vector2i& dim) {
    unsigned minimumImageDim = std::min(imageInfo.width(), imageInfo.height());
    unsigned minimumRectDim = std::min(dim.x(), dim.y());
    unsigned imDim = std::max<unsigned>(minimumRectDim, SMALLEST_MIPMAP_RESOLUTION);
    unsigned divisionLevel = minimumImageDim / imDim;
    return (divisionLevel <= 1) ? 0 : (std::bit_width(divisionLevel) - 1);
}

unsigned ImageResourceDisplay::get_best_allocated_mipmap_level(unsigned mipmapLevel) {
    if(mipmapLevel == get_smallest_mipmap_level())
        return mipmapLevel;
    constexpr static unsigned NUMERIC_UNSIGNED_MAX = std::numeric_limits<unsigned>::max();
    for(unsigned i = mipmapLevel; i != NUMERIC_UNSIGNED_MAX; i--) {
        if(mipmapLevelsStatus[i] != MipmapLevelStatus::UNALLOCATED)
            return i;
    }
    for(unsigned i = mipmapLevel; i < get_smallest_mipmap_level(); i++) {
        if(mipmapLevelsStatus[i] != MipmapLevelStatus::UNALLOCATED)
            return i;
    }
    return get_smallest_mipmap_level();
}

unsigned ImageResourceDisplay::calculate_smallest_mipmap_level() {
    unsigned minimumImageDim = std::min(imageInfo.width(), imageInfo.height());
    unsigned divisionLevel = minimumImageDim / SMALLEST_MIPMAP_RESOLUTION;
    return (divisionLevel <= 1) ? 0 : std::bit_width(divisionLevel - 1);
}

unsigned ImageResourceDisplay::get_smallest_mipmap_level() const {
    return mipmapLevelsStatus.size();
}

Vector2i ImageResourceDisplay::get_mipmap_level_image_dimensions(unsigned mipmapLevel) const {
    unsigned divideResolutionBy = 1 << mipmapLevel;
    return {imageInfo.width() / divideResolutionBy, imageInfo.height() / divideResolutionBy};
}

Vector2f ImageResourceDisplay::get_dimensions() const {
    return {imageInfo.width(), imageInfo.height()};
}

float ImageResourceDisplay::get_dimension_scale() const {
    return 0.3f;
}

ResourceDisplay::Type ImageResourceDisplay::get_type() const {
    return ResourceDisplay::Type::IMAGE;
}

ImageResourceDisplay::~ImageResourceDisplay() {
    if(loadThread) {
        shutdownLoadThread = true;
        loadThread->join();
    }
}
