#include "DrawCamera.hpp"
#include "DrawingProgram/Tools/DrawingProgramToolBase.hpp"
#include "Helpers/FixedPoint.hpp"
#include "Helpers/MathExtras.hpp"
#include "MainProgram.hpp"
#include "World.hpp"
#include "InputManager.hpp"
#include <Helpers/Logger.hpp>

DrawCamera::DrawCamera():
    c({0, 0}, WorldScalar(1000000), 0.0)
{}

void DrawCamera::set_based_on_properties(World& w, const WorldVec& newPos, const WorldScalar& newZoom, double newRotate) {
    c.inverseScale = newZoom;
    c.pos = newPos;
    c.set_rotation(newRotate);

    set_viewing_area(w.main.window.size.cast<float>());
}

void DrawCamera::set_based_on_center(World& w, const WorldVec& newPos, const WorldScalar& newZoom, double newRotate) {
    c.inverseScale = newZoom;
    c.set_rotation(newRotate);
    c.pos = newPos - c.dir_from_space(w.main.window.size.cast<float>() * 0.5f);

    set_viewing_area(w.main.window.size.cast<float>());
}

void DrawCamera::set_viewing_area(Vector2f viewingAreaNew) {
    if(viewingArea != viewingAreaNew) {
        viewingArea = viewingAreaNew;
    }
    float maxDim = std::max(viewingAreaNew.x(), viewingAreaNew.y());
    WorldScalar a = WorldScalar(maxDim * 0.708f) * c.inverseScale;
    WorldVec center = c.from_space(viewingAreaNew * 0.5f);
    viewingAreaGenerousCollider = SCollision::AABB<WorldScalar>(center - WorldVec{a, a}, center + WorldVec{a, a});
}

void DrawCamera::smooth_move_to(World& w, const CoordSpaceHelper& newCoords, Vector2f windowSize, bool instantJump) {
    smoothMove.start = c;
    smoothMove.end = newCoords;
    smoothMove.endWindowSize = windowSize;
    smoothMove.startCenter = c.pos + c.dir_from_space(w.main.window.size.cast<float>() * 0.5f);
    smoothMove.endCenter = newCoords.pos + newCoords.dir_from_space(windowSize * 0.5f);

    float a1(w.main.window.size.x() / windowSize.x());
    float a2(w.main.window.size.y() / windowSize.y());
    smoothMove.endUniformZoom = std::max(smoothMove.end.inverseScale.multiply_double((a1 < a2) ? a1 : a2), WorldScalar(1));

    smoothMove.occurring = true;
    smoothMove.moveTime = (instantJump || w.main.toolbar.jumpTransitionTime <= 0.01f) ? w.main.toolbar.jumpTransitionTime : 0.0f;
}

void DrawCamera::scale_up(World& w, const WorldScalar& scaleUpAmount) {
    smoothMove.occurring = false;
    c.scale_about({0, 0}, scaleUpAmount, true);
    startZoomVal *= scaleUpAmount;
    startZoomMousePos *= scaleUpAmount;
    startZoomCameraPos *= scaleUpAmount;
    set_viewing_area(w.main.window.size.cast<float>());
}

void DrawCamera::update_main(World& w) {
    if(smoothMove.occurring) {
        BezierEasing zoomAnim{w.main.toolbar.jumpTransitionEasing};
        float smoothTime = smooth_two_way_time(smoothMove.moveTime, w.main.deltaTime, true, w.main.toolbar.jumpTransitionTime);
        float lerpTime;
        float rotationLerpTime = zoomAnim(smoothTime);
        lerpTime = zoomAnim(smoothTime);

        WorldVec mVec = smoothMove.startCenter - smoothMove.endCenter;
        WorldScalar startLog2 = FixedPoint::log2(smoothMove.start.inverseScale);
        WorldScalar endLog2 = FixedPoint::log2(smoothMove.endUniformZoom);
        WorldVec windowCenter = smoothMove.endCenter;
        if(FixedPoint::abs(startLog2 - endLog2) < WorldScalar(3)) {
            c.inverseScale = FixedPoint::lerp_double(smoothMove.start.inverseScale, smoothMove.endUniformZoom, lerpTime);
            if(lerpTime != 1.0)
                windowCenter += WorldVec{mVec.x() / WorldScalar(1.0 / (1.0 - lerpTime)), mVec.y() / WorldScalar(1.0 / (1.0 - lerpTime))};
            c.pos = windowCenter - (w.main.window.size.cast<float>() * 0.5f).cast<WorldScalar>() * c.inverseScale;
        }
        else {
            c.inverseScale = FixedPoint::exp2(FixedPoint::lerp_double(startLog2, endLog2, lerpTime));
            if(smoothMove.start.inverseScale < smoothMove.endUniformZoom) {
                if(smoothMove.start.inverseScale > c.inverseScale)
                    c.inverseScale = smoothMove.start.inverseScale;
                else if(smoothMove.endUniformZoom < c.inverseScale)
                    c.inverseScale = smoothMove.endUniformZoom;
            }
            else {
                if(smoothMove.start.inverseScale < c.inverseScale)
                    c.inverseScale = smoothMove.start.inverseScale;
                else if(smoothMove.endUniformZoom > c.inverseScale)
                    c.inverseScale = smoothMove.endUniformZoom;
            }
            WorldScalar denominator = (smoothMove.endUniformZoom - smoothMove.start.inverseScale);
            windowCenter += WorldVec{mVec.x() - ((mVec.x() * c.inverseScale) / denominator) + ((mVec.x() * smoothMove.start.inverseScale) / denominator),
                                     mVec.y() - ((mVec.y() * c.inverseScale) / denominator) + ((mVec.y() * smoothMove.start.inverseScale) / denominator)};
            c.pos = windowCenter - (w.main.window.size.cast<float>() * 0.5f).cast<WorldScalar>() * c.inverseScale;
        }

        c.set_rotation(0.0);
        double midRotate = Rotation2D(smoothMove.start.rotation).slerp(rotationLerpTime, Rotation2D(smoothMove.end.rotation)).smallestPositiveAngle();
        c.rotate_about(windowCenter, midRotate);

        if(smoothTime >= 1.0f) {
            c.inverseScale = smoothMove.endUniformZoom;
            c.pos = smoothMove.endCenter - (w.main.window.size.cast<float>() * 0.5f).cast<WorldScalar>() * c.inverseScale;
            c.set_rotation(0.0);
            c.rotate_about(smoothMove.endCenter, smoothMove.end.rotation);
            smoothMove.occurring = false;
        }
    }
    else {
        if(w.main.input.key(InputManager::KEY_CAMERA_ROTATE_COUNTERCLOCKWISE).held && !w.main.input.text.is_accepting_input()) {
            c.rotate_about(c.from_space(w.main.window.size.cast<float>() * 0.5f), -w.main.deltaTime);
            InputManager::MouseMotionCallbackArgs motion{
                .pos = w.main.input.mouse.pos,
                .move = {0, 0}
            };
            w.drawProg.drawTool->input_mouse_motion_callback(motion);
        }
        if(w.main.input.key(InputManager::KEY_CAMERA_ROTATE_CLOCKWISE).held && !w.main.input.text.is_accepting_input()) {
            c.rotate_about(c.from_space(w.main.window.size.cast<float>() * 0.5f), w.main.deltaTime);
            InputManager::MouseMotionCallbackArgs motion{
                .pos = w.main.input.mouse.pos,
                .move = {0, 0}
            };
            w.drawProg.drawTool->input_mouse_motion_callback(motion);
        }
    }

    set_viewing_area(w.main.window.size.cast<float>());

    if(c.inverseScale < WorldScalar(1))
        w.scale_up_step();
}

void DrawCamera::input_key_callback(const InputManager::KeyCallbackArgs& key) {
}

void DrawCamera::input_mouse_button_on_canvas_callback(World& w, const InputManager::MouseButtonCallbackArgs& button) {
    if(!smoothMove.occurring && !isTouchTransforming) {
        bool newIsAccurateZooming = (w.drawProg.controls.middleClickHeld && w.main.input.pen.isDown && w.main.toolbar.tabletOptions.zoomWhilePenDownAndButtonHeld) || // Hold middle click (pen button assigned to middle click) while pen is down
                                    (w.drawProg.controls.middleClickHeld && w.main.input.key(InputManager::KEY_GENERIC_LCTRL).held) || // Hold middle click/pen button while holding control
                                    (w.drawProg.controls.leftClickHeld && w.drawProg.drawTool->get_type() == DrawingProgramToolType::ZOOM); // Hold left click while on zoom tool
        if(newIsAccurateZooming && !isAccurateZooming) {
            startZoomMousePos = c.from_space(button.pos);
            startZoomVal = c.inverseScale;
            startZoomCameraPos = c.pos;
        }
        isAccurateZooming = newIsAccurateZooming;
    }
    else
        isAccurateZooming = false;
}

void DrawCamera::input_mouse_motion_callback(World& w, const InputManager::MouseMotionCallbackArgs& motion) {
    if(!smoothMove.occurring && !isTouchTransforming) {
        if(isAccurateZooming && startZoomVal != WorldScalar(0)) {
            WorldScalar zoomFactor(std::pow(1.0 + w.main.toolbar.dragZoomSpeed, w.main.toolbar.flipZoomToolDirection ? motion.move.y() : -motion.move.y()));
            if(zoomFactor < WorldScalar(0.000001))
                zoomFactor = WorldScalar(0.000001);

            c.scale(zoomFactor);
            if(c.inverseScale < WorldScalar(0.0001))
                c.inverseScale = WorldScalar(0.0001);
            else {
                WorldVec mVec = startZoomCameraPos - startZoomMousePos;
                WorldScalar mX = static_cast<WorldScalar>(WorldMultiplier(c.inverseScale) / WorldMultiplier(startZoomVal));
                c.pos = startZoomMousePos + mVec * mX;
            }
        }
        else if(w.drawProg.controls.middleClickHeld || (w.drawProg.controls.leftClickHeld && w.drawProg.drawTool->get_type() == DrawingProgramToolType::PAN))
            c.pos -= c.dir_from_space(motion.move);
    }
}

void DrawCamera::input_mouse_wheel_callback(World& w, const InputManager::MouseWheelCallbackArgs& wheel) {
    if(!smoothMove.occurring && !isTouchTransforming && wheel.tickAmount.y() && w.main.toolbar.check_if_position_isnt_obstructed(wheel.mousePos)) {
        WorldVec mouseWorldPos = c.from_space(wheel.mousePos);
        WorldScalar zoomFactor(1.0 + w.main.toolbar.scrollZoomSpeed);

        if(zoomFactor < WorldScalar(0.000001))
            zoomFactor = WorldScalar(0.000001);

        if(zoomFactor != WorldScalar(0)) {
            if(wheel.tickAmount.y() < 0.0)
                zoomFactor = WorldScalar(1) / zoomFactor;

            for(int i = 0; i < std::abs(wheel.tickAmount.y()); i++) {
                c.scale(zoomFactor);
                if(c.inverseScale < WorldScalar(0.0001))
                    c.inverseScale = WorldScalar(0.0001);
                else {
                    WorldVec mVec = c.pos - mouseWorldPos;
                    c.pos = mouseWorldPos + mVec / zoomFactor;
                }
            }
        }
    }
}

void DrawCamera::input_multi_finger_touch_callback(World& w, const InputManager::MultiFingerTouchArgs& touch) {
    if(!smoothMove.occurring && !isAccurateZooming && !isTouchTransforming && touch.down) {
        touchInitialPositions = touch.pos;
        touchInitialC = c;
        isTouchTransforming = true;
    }
    else
        isTouchTransforming = false;
}

void DrawCamera::input_multi_finger_motion_callback(World& w, const InputManager::MultiFingerMotionArgs& motion) {
    if(!smoothMove.occurring && !isAccurateZooming && isTouchTransforming) {
        c = touchInitialC;

        Vector2f initialCenter = (touchInitialPositions[0] + touchInitialPositions[1]) * 0.5f;
        Vector2f newCenter = (motion.pos[0] + motion.pos[1]) * 0.5f;
        c.pos -= c.dir_from_space(newCenter - initialCenter);
        WorldVec newCenterWorld = c.from_space(newCenter);

        float initialDistance = vec_distance(touchInitialPositions[0], touchInitialPositions[1]);
        float newDistance = vec_distance(motion.pos[0], motion.pos[1]);
        float scaleAmount = newDistance / initialDistance;
        c.scale_about_double(newCenterWorld, scaleAmount);

        Vector2f initialDiff = touchInitialPositions[0] - initialCenter;
        float initialAngle = std::atan2(initialDiff.y(), initialDiff.x()) + std::numbers::pi;
        Vector2f newDiff = motion.pos[0] - newCenter;
        float newAngle = std::atan2(newDiff.y(), newDiff.x()) + std::numbers::pi;
        float rotateAngle = initialAngle - newAngle;
        rotateAngle = std::fmod(rotateAngle + std::numbers::pi, std::numbers::pi * 2.0f) - std::numbers::pi;
        c.rotate_about(newCenterWorld, rotateAngle);
    }
}

void DrawCamera::save_file(cereal::PortableBinaryOutputArchive& a, const World& w) const {
    a(c, w.main.window.size.cast<float>().eval());
}

void DrawCamera::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version, World& w) {
    CoordSpaceHelper coordsToJumpTo;
    Vector2f windowSizeToJumpTo;
    a(coordsToJumpTo, windowSizeToJumpTo);
    smooth_move_to(w, coordsToJumpTo, windowSizeToJumpTo, true);
}
