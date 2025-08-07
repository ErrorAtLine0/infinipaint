#include "ImageResourceDisplay.hpp"
#include <include/codec/SkPngDecoder.h>
#include <include/codec/SkJpegDecoder.h>
#include <include/codec/SkBmpDecoder.h>
#include <include/codec/SkIcoDecoder.h>
#include <include/codec/SkGifDecoder.h>
#include "../MainProgram.hpp"
#include <Helpers/Logger.hpp>

std::array<const SkCodecs::Decoder, 5> decoders = {
    SkPngDecoder::Decoder(),
    SkJpegDecoder::Decoder(),
    SkBmpDecoder::Decoder(),
    SkIcoDecoder::Decoder(),
    SkGifDecoder::Decoder()
};

bool ImageResourceDisplay::update_draw() const {
    return mustUpdateDraw;
}

void ImageResourceDisplay::update(World& w) {
    if(frames.size() <= 1)
        return;
    currentTime += w.main.deltaTime * 1000.0;
    mustUpdateDraw = false;
    for(;;) {
        if(currentTime >= frames[frameIndex].duration) {
            currentTime -= frames[frameIndex].duration;
            frameIndex++;
            if(frameIndex >= frames.size())
                frameIndex = 0;
            mustUpdateDraw = true;
        }
        else
            return;
        if(frames[frameIndex].duration <= 0.0f)
            return;
    }
}

bool ImageResourceDisplay::load(ResourceManager& rMan, const std::string& fileName, const std::string& fileData) {
    sk_sp<SkData> newData = SkData::MakeWithoutCopy(fileData.c_str(), fileData.size());
    std::unique_ptr<SkCodec> codec = SkCodec::MakeFromData(newData, decoders);
    mustUpdateDraw = true;
    if(codec) {
        auto origin = codec->getOrigin();
        bool rotate = (origin == kLeftTop_SkEncodedOrigin || origin == kRightTop_SkEncodedOrigin || origin == kRightBottom_SkEncodedOrigin || origin == kLeftBottom_SkEncodedOrigin);
        int frameCount = codec->getFrameCount();
        SkImageInfo imageInfo = codec->getInfo();
        if(rotate) {
            Logger::get().log("INFO", "Image loaded is rotated, so must rotate dimensions");
            imageInfo = imageInfo.makeDimensions(SkISize(imageInfo.height(), imageInfo.width()));
        }
        for(int i = 0; i < frameCount; i++) {
            SkCodec::Options imageOptions;
            imageOptions.fFrameIndex = i;
            SkCodec::FrameInfo frameInfo;
            float frameTime = -1.0f;
            if(codec->getFrameInfo(i, &frameInfo))
                frameTime = frameInfo.fDuration;
            auto decodeResult = codec->getImage(imageInfo, &imageOptions);
            if(std::get<1>(decodeResult) == SkCodec::Result::kSuccess)
                frames.emplace_back(std::get<0>(decodeResult)->withDefaultMipmaps(), frameTime);
            else
                throw std::runtime_error("Could not decode image, got error code " + std::to_string(std::get<1>(decodeResult)) + ". Image has dimensions " + std::to_string(imageInfo.width()) + " " + std::to_string(imageInfo.height()));
        }
        return true;
    }
    return false;
}

void ImageResourceDisplay::draw(SkCanvas* canvas, const DrawData& drawData, const SkRect& imRect) {
    canvas->drawImageRect(frames[frameIndex].data, imRect, {SkFilterMode::kLinear, SkMipmapMode::kLinear});
}

Vector2f ImageResourceDisplay::get_dimensions() const {
    return {frames.front().data->width(), frames.front().data->height()};
}

float ImageResourceDisplay::get_dimension_scale() const {
    return 0.3f;
}
