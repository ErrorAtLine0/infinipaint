#include "FingerInputTracker.hpp"
#include "Helpers/MathExtras.hpp"
#include <chrono>

namespace FingerInput {

constexpr float MAX_DELTA_MOTION_TO_DISABLE_TAP_SQRD = 20.0f * 20.0f;
constexpr float MAX_DELTA_INITIAL_TO_DISABLE_TAP_SQRD = 20.0f * 20.0f;
constexpr std::chrono::duration DURATION_TO_HOLD = std::chrono::milliseconds(500);
constexpr std::chrono::duration MAX_DURATION_BETWEEN_TAPS = std::chrono::milliseconds(500);

void FingerData::scale(float multiplier) {
    pos /= multiplier;
    initialTouchPos /= multiplier;
}

GestureType PreTapGesture::get_type() const { return GestureType::PRETAP; }
GestureType TapGesture::get_type() const { return GestureType::TAP; }
GestureType HoldGesture::get_type() const { return GestureType::HOLD; }

std::unique_ptr<BaseGesture> PreTapGesture::clone() const {
    auto toRet = std::make_unique<PreTapGesture>();
    *toRet = *this;
    return toRet;
}

void PreTapGesture::scale(float multiplier) {
    for(Vector2f& f : fingerPositions)
        f /= multiplier;
}

std::vector<Vector2f> PreTapGesture::get_all_positions() const {
    return fingerPositions;
}

std::unique_ptr<BaseGesture> TapGesture::clone() const {
    auto toRet = std::make_unique<TapGesture>();
    *toRet = *this;
    return toRet;
}

void TapGesture::scale(float multiplier) {
    for(Vector2f& f : fingerPositions)
        f /= multiplier;
}

std::vector<Vector2f> TapGesture::get_all_positions() const {
    return fingerPositions;
}

std::unique_ptr<BaseGesture> HoldGesture::clone() const {
    auto toRet = std::make_unique<HoldGesture>();
    *toRet = *this;
    return toRet;
}

void HoldGesture::scale(float multiplier) {
    fingerPosition /= multiplier;
}

std::vector<Vector2f> HoldGesture::get_all_positions() const {
    return {fingerPosition};
}

TouchCallbackArgs TouchCallbackArgs::clone() const {
    TouchCallbackArgs toRet;
    toRet.fingers = fingers;
    toRet.action = action;
    if(gesture)
        toRet.gesture = gesture->clone();
    return toRet;
}

TouchCallbackArgs TouchCallbackArgs::scaled_clone(float multiplier) const {
    auto toRet = clone();
    for(FingerData& f : toRet.fingers)
        f.scale(multiplier);
    toRet.action.motion /= multiplier;
    toRet.action.pos /= multiplier;
    if(toRet.gesture)
        toRet.gesture->scale(multiplier);
    return toRet;
}

TouchCallbackArgs InputTracker::update_finger_data_input_callback(SDL_EventType eventType, SDL_TouchID touchDeviceID, SDL_FingerID fingerID, const Vector2f& pos, const Vector2f& delta) {
    switch(eventType) {
        case SDL_EVENT_FINGER_DOWN: {
            auto touchTime = std::chrono::steady_clock::now();
            bool isFirstFingerDown = fingers.empty();
            if(tap.fingersGoingUp)
                invalidate_tap();
            else if(touchTime - tap.lastTapTime > MAX_DURATION_BETWEEN_TAPS)
                reset_tap();
            fingers.emplace_back(FingerData{
                .fingerID = fingerID,
                .pos = pos,
                .initialTouchPos = pos,
                .initialTouchTime = touchTime,
                .isFirstFingerDown = isFirstFingerDown
            });
            break;
        }
        case SDL_EVENT_FINGER_MOTION: {
            auto f = std::find_if(fingers.begin(), fingers.end(), [&](const FingerData& f) { return fingerID == f.fingerID; });
            if(f != fingers.end()) {
                f->pos = pos;
                if(vec_distance_sqrd(f->pos, f->initialTouchPos) > MAX_DELTA_MOTION_TO_DISABLE_TAP_SQRD) {
                    f->fingerMovedAlot = true;
                    invalidate_tap();
                }
            }
            break;
        }
        default: break;
    }
    TouchCallbackArgs toRet;
    toRet.fingers = fingers;
    toRet.action = {
        .type = ActionType::NONE,
        .fingerID = fingerID,
        .pos = pos,
        .motion = delta
    };
    switch(eventType) {
        case SDL_EVENT_FINGER_UP: {
            toRet.action.type = ActionType::UP;
            if(!tap.fingersGoingUp) {
                if(fingers.size() != tap.fingerCount) {
                    tap.count = 0;
                    tap.fingerCount = fingers.size();
                }
                tap.positions.clear();
                for(const FingerData& f : fingers)
                    tap.positions.emplace_back(f.pos);
            }
            tap.fingersGoingUp = true;
            std::erase_if(fingers, [&](const FingerData& f) { return fingerID == f.fingerID; });
            if(fingers.empty()) {
                if(tap.invalid) {
                    reset_tap();
                    tap.invalid = false;
                }
                else {
                    auto tapGesture = std::make_unique<TapGesture>();
                    tap.count++;
                    tapGesture->fingerPositions = tap.positions;
                    tapGesture->numberOfTaps = tap.count;
                    toRet.gesture = std::move(tapGesture);
                }
                tap.fingersGoingUp = false;
            }
            break;
        }
        case SDL_EVENT_FINGER_DOWN: {
            if(!tap.invalid && tap.fingerCount == fingers.size() && tap.count >= 1) {
                auto pretapGesture = std::make_unique<PreTapGesture>();
                pretapGesture->fingerPositions = tap.positions;
                pretapGesture->numberOfTaps = tap.count + 1;
                toRet.gesture = std::move(pretapGesture);
            }
            toRet.action.type = ActionType::DOWN;
            break;
        }
        case SDL_EVENT_FINGER_MOTION:
            toRet.action.type = ActionType::MOVE;
            break;
        default: break;
    }
    return toRet;
}

void InputTracker::update() {
    if(!fingers.empty() && (std::chrono::steady_clock::now() - fingers[0].initialTouchTime) >= DURATION_TO_HOLD && fingers[0].isFirstFingerDown) {
        // HOLD GESTURE
    }
}

void InputTracker::invalidate_tap() {
    reset_tap();
    tap.invalid = true;
}

void InputTracker::reset_tap() {
    tap.count = 0;
    tap.fingerCount = 0;
    tap.positions.clear();
}

}
