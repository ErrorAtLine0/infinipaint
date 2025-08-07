#pragma once
#include <Eigen/Dense>
#include <Helpers/ConvertVec.hpp>
#include "SharedTypes.hpp"
#include <Helpers/SCollision.hpp>
#include "CoordSpaceHelper.hpp"

#include "include/core/SkMatrix.h"

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
        bool isMiddleClickZooming = false;
};
