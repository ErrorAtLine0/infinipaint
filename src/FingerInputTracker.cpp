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

GestureType TapGesture::get_type() const { return GestureType::TAP; }
GestureType HoldGesture::get_type() const { return GestureType::HOLD; }

void TapGesture::scale(float multiplier) {
    for(Vector2f& f : fingerPositions)
        f /= multiplier;
}

std::vector<Vector2f> TapGesture::get_all_positions() const {
    return fingerPositions;
}

void HoldGesture::scale(float multiplier) {
    fingerPosition /= multiplier;
}

std::vector<Vector2f> HoldGesture::get_all_positions() const {
    return {fingerPosition};
}

void TouchCallbackArgs::scale(float multiplier) {
    for(FingerData& f : fingers)
        f.scale(multiplier);
    action.motion /= multiplier;
    action.pos /= multiplier;
    if(gesture)
        gesture->scale(multiplier);
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
                    auto tapGesture = std::make_shared<TapGesture>();
                    tap.count++;
                    tapGesture->fingerPositions = tap.positions;
                    tapGesture->numberOfTaps = tap.count;
                    toRet.gesture = tapGesture;
                }
                tap.fingersGoingUp = false;
            }
            break;
        }
        case SDL_EVENT_FINGER_DOWN:
            toRet.action.type = ActionType::DOWN;
            break;
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
