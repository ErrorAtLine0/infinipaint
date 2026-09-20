#pragma once
#include <SDL3/SDL.h>
#include "CustomEvents.hpp"

namespace FingerInput {

struct FingerData {
    void scale(float multiplier);
    SDL_FingerID fingerID;
    Vector2f pos;

    Vector2f initialTouchPos;
    std::chrono::steady_clock::time_point initialTouchTime;
    bool fingerMovedAlot = false;
    bool isFirstFingerDown = false;
};

enum class ActionType {
    NONE,
    DOWN,
    UP,
    MOVE
};

enum class GestureType {
    NONE,
    TAP, // Carries number of fingers, and number of taps
    HOLD // Finger down, no movement for a long time. Hold position is position of first finger down
};

class BaseGesture {
    public:
        virtual void scale(float multiplier) = 0;
        virtual GestureType get_type() const = 0;
        virtual std::vector<Vector2f> get_all_positions() const = 0;
};

class TapGesture : public BaseGesture {
    public:
        virtual void scale(float multiplier) override;
        virtual GestureType get_type() const override;
        virtual std::vector<Vector2f> get_all_positions() const override;
        std::vector<Vector2f> fingerPositions;
        unsigned numberOfTaps;
};

class HoldGesture : public BaseGesture {
    public:
        virtual void scale(float multiplier) override;
        virtual GestureType get_type() const override;
        virtual std::vector<Vector2f> get_all_positions() const override;
        Vector2f fingerPosition;
};

struct TouchCallbackArgs {
    void scale(float multiplier);
    std::vector<FingerData> fingers;
    struct {
        ActionType type;
        SDL_FingerID fingerID;
        Vector2f pos;
        Vector2f motion;
    } action;
    std::shared_ptr<BaseGesture> gesture;
};

class InputTracker {
    public:
        TouchCallbackArgs update_finger_data_input_callback(SDL_EventType eventType, SDL_TouchID touchDeviceID, SDL_FingerID fingerID, const Vector2f& pos, const Vector2f& delta);
        void update();
    private:
        struct {
            unsigned count = 0;
            unsigned fingerCount = 0;
            bool fingersGoingUp = false;
            bool invalid = false;
            std::vector<Vector2f> positions;
            std::chrono::steady_clock::time_point lastTapTime;
            std::chrono::steady_clock::time_point firstFingerDownInCurrentTapTime;
        } tap;
        void invalidate_tap();
        void reset_tap();
        std::vector<FingerData> fingers;
};

}
