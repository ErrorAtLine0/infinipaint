#include "ImageResourceDisplay.hpp"
#include <include/codec/SkPngDecoder.h>
#include <include/codec/SkJpegDecoder.h>
#include <include/codec/SkBmpDecoder.h>
#include <include/codec/SkIcoDecoder.h>
#include <include/codec/SkGifDecoder.h>
//#include <include/codec/SkRawDecoder.h>
#include <include/codec/SkWbmpDecoder.h>
#include <include/codec/SkWebpDecoder.h>
#include "../MainProgram.hpp"
#include <Helpers/Logger.hpp>

std::array<const SkCodecs::Decoder, 7> decoders = {
    SkBmpDecoder::Decoder(),
    SkGifDecoder::Decoder(),
    SkIcoDecoder::Decoder(),
    SkJpegDecoder::Decoder(),
    SkPngDecoder::Decoder(),
//    SkRawDecoder::Decoder(),
    SkWbmpDecoder::Decoder(),
    SkWebpDecoder::Decoder()
};

void ImageResourceDisplay::load_thread() {
    for(int i = 0; i < totalFramesToLoad; i++) {
        if(shutdownLoadThread)
            break;
        SkCodec::Options imageOptions;
        imageOptions.fFrameIndex = i;
        SkCodec::FrameInfo frameInfo;
        float frameTime = -1.0f;
        if(codec->getFrameInfo(i, &frameInfo))
            frameTime = frameInfo.fDuration;
        auto decodeResult = codec->getImage(imageInfo, &imageOptions);
        if(std::get<1>(decodeResult) == SkCodec::Result::kSuccess)
            frames[i] = {std::get<0>(decodeResult)->withDefaultMipmaps(), frameTime};
        else {
            Logger::get().log("WORLDFATAL", "Could not decode image, got error code " + std::to_string(std::get<1>(decodeResult)) + ". Image has dimensions " + std::to_string(imageInfo.width()) + " " + std::to_string(imageInfo.height()));
            break;
        }
        loadedFrames++;
    }
    imageRawData = nullptr;
    codec = nullptr;
}

bool ImageResourceDisplay::update_draw() const {
    return mustUpdateDraw;
}

void ImageResourceDisplay::update(World& w) {
    mustUpdateDraw = false;
    if(!loadedFirstFrame && loadedFrames >= 1) {
        mustUpdateDraw = true;
        loadedFirstFrame = true;
    }
    if(loadedFrames <= 1)
        return;
    currentTime += w.main.deltaTime * 1000.0;
    for(;;) {
        if(currentTime >= frames[frameIndex].duration) {
            currentTime -= frames[frameIndex].duration;
            frameIndex++;
            if(frameIndex >= loadedFrames) {
                if(loadedFrames != totalFramesToLoad) // Dont loop the GIF if it's not completely loaded yet
                    frameIndex--;
                else
                    frameIndex = 0;
            }
            mustUpdateDraw = true;
        }
        else
            return;
        if(frames[frameIndex].duration <= 0.0f)
            return;
    }
}

bool ImageResourceDisplay::load(ResourceManager& rMan, const std::string& fileName, const std::string& fileData) {
    imageRawData = SkData::MakeWithoutCopy(fileData.c_str(), fileData.size());
    codec = SkCodec::MakeFromData(imageRawData, decoders);
    mustUpdateDraw = true;
    if(codec) {
        auto origin = codec->getOrigin();
        bool rotate = (origin == kLeftTop_SkEncodedOrigin || origin == kRightTop_SkEncodedOrigin || origin == kRightBottom_SkEncodedOrigin || origin == kLeftBottom_SkEncodedOrigin);
        totalFramesToLoad = codec->getFrameCount();
        imageInfo = codec->getInfo();
        frames.resize(totalFramesToLoad);
        loadedFrames = 0;
        if(rotate) {
            Logger::get().log("INFO", "Image loaded is rotated, so must rotate dimensions");
            imageInfo = imageInfo.makeDimensions(SkISize(imageInfo.height(), imageInfo.width()));
        }
        try{
            loadThread = std::make_unique<std::thread>(&ImageResourceDisplay::load_thread, this);
        }
        catch(...) {
            Logger::get().log("INFO", "[ImageResourceDisplay::load] Failed to create load thread, loading on main thread");
            load_thread();
        }
        return true;
    }
    return false;
}

void ImageResourceDisplay::draw(SkCanvas* canvas, const DrawData& drawData, const SkRect& imRect) {
    if(loadedFrames == 0)
        canvas->drawRect(imRect, SkPaint({0.5f, 0.5f, 0.5f, 0.5f}));
    else
        canvas->drawImageRect(frames[frameIndex].data, imRect, {SkFilterMode::kLinear, SkMipmapMode::kLinear});
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
