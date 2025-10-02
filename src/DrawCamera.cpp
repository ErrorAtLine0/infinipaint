#include "DrawCamera.hpp"
#include "DrawingProgram/DrawingProgramToolBase.hpp"
#include "Helpers/FixedPoint.hpp"
#include "Helpers/MathExtras.hpp"
#include "MainProgram.hpp"
#include "Server/CommandList.hpp"
#include "World.hpp"
#include "InputManager.hpp"
#include <iostream>

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
        bool newIsAccurateZooming = (w.drawProg.controls.middleClickHeld && w.main.input.key(InputManager::KEY_GENERIC_LCTRL).held) || (w.drawProg.controls.leftClickHeld && w.drawProg.drawTool->get_type() == DrawingProgramToolType::ZOOM);

        if(newIsAccurateZooming && !isAccurateZooming) {
            startZoomMousePos = w.get_mouse_world_pos();
            startZoomVal = c.inverseScale;
            startZoomCameraPos = c.pos;
        }
        isAccurateZooming = newIsAccurateZooming;
        if(isAccurateZooming) {
            WorldScalar zoomFactor(std::pow(1.0 + w.main.toolbar.dragZoomSpeed, -w.main.input.mouse.move.y()));
            c.scale(zoomFactor);
            WorldVec mVec = startZoomCameraPos - startZoomMousePos;
            WorldScalar mX = c.inverseScale / startZoomVal;
            c.pos = startZoomMousePos + mVec * mX;
        }
        else if(w.drawProg.controls.middleClickHeld || (w.drawProg.controls.leftClickHeld && w.drawProg.drawTool->get_type() == DrawingProgramToolType::PAN))
            c.pos -= c.dir_from_space(w.main.input.mouse.move);
        if(w.main.input.mouse.scrollAmount.y() && !w.main.toolbar.io->hoverObstructed) {
            // Ignore the value of scrollAmount, since that leads to problems on macOS

            WorldScalar zoomFactor(1.0 + w.main.toolbar.scrollZoomSpeed);
            if(w.main.input.mouse.scrollAmount.y() < 0.0)
                zoomFactor = WorldScalar(1) / zoomFactor;

            c.scale(zoomFactor);
            WorldVec mVec = c.pos - w.get_mouse_world_pos();
            c.pos = w.get_mouse_world_pos() + mVec / zoomFactor;
        }
        if(w.main.input.key(InputManager::KEY_CAMERA_ROTATE_COUNTERCLOCKWISE).held && !w.main.input.text.get_accepting_input()) {
            c.rotate_about(c.from_space(w.main.window.size.cast<float>() * 0.5f), -w.main.deltaTime);
        }
        if(w.main.input.key(InputManager::KEY_CAMERA_ROTATE_CLOCKWISE).held && !w.main.input.text.get_accepting_input()) {
            c.rotate_about(c.from_space(w.main.window.size.cast<float>() * 0.5f), w.main.deltaTime);
        }
    }

    set_viewing_area(w.main.window.size.cast<float>());

    if(c.inverseScale < WorldScalar(1))
        w.scale_up_step();
}
