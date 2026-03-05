#pragma once
#include <Eigen/Dense>
#include <Helpers/ConvertVec.hpp>
#include "SharedTypes.hpp"
#include <Helpers/SCollision.hpp>
#include "CoordSpaceHelper.hpp"
#include <Helpers/VersionNumber.hpp>
#include "InputManager.hpp"

using namespace Eigen;

class World;

class DrawCamera {
    public:
        DrawCamera();

        CoordSpaceHelper c;
        Vector2f viewingArea{-1, -1};
        SCollision::AABB<WorldScalar> viewingAreaGenerousCollider;

        void set_viewing_area(Vector2f viewingAreaNew);
        void smooth_move_to(World& w, const CoordSpaceHelper& c, Vector2f windowSize, bool instantJump = false);
        void set_based_on_properties(World& w, const WorldVec& newPos, const WorldScalar& newZoom, double newRotate);
        void set_based_on_center(World& w, const WorldVec& newPos, const WorldScalar& newZoom, double newRotate);
        void update_main(World& main);

        void scale_up(World& w, const WorldScalar& scaleUpAmount);

        void save_file(cereal::PortableBinaryOutputArchive& a, const World& w) const;
        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version, World& w);

        void input_key_callback(const InputManager::KeyCallbackArgs& key);
        void input_mouse_button_on_canvas_callback(World& w, const InputManager::MouseButtonCallbackArgs& button);
        void input_mouse_motion_callback(World& w, const InputManager::MouseMotionCallbackArgs& motion);
        void input_mouse_wheel_callback(World& w, const InputManager::MouseWheelCallbackArgs& wheel);
        void input_multi_finger_touch_callback(World& w, const InputManager::MultiFingerTouchCallbackArgs& touch);
        void input_multi_finger_motion_callback(World& w, const InputManager::MultiFingerMotionCallbackArgs& motion);
    private:
        struct SmoothMove {
            CoordSpaceHelper start;
            WorldVec startCenter;
            WorldVec endCenter;
            WorldScalar endUniformZoom;
            CoordSpaceHelper end;
            Vector2f endWindowSize;
            bool occurring = false;
            float moveTime;
        } smoothMove;

        WorldScalar startZoomVal;
        WorldVec startZoomMousePos;
        WorldVec startZoomCameraPos;
        bool isAccurateZooming = false;

        CoordSpaceHelper touchInitialC;
        std::vector<Vector2f> touchInitialPositions;
        bool isTouchTransforming = false;
};
