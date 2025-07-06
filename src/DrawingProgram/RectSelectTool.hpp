#pragma once
#include <include/core/SkCanvas.h>
#include "../DrawData.hpp"
#include <Helpers/SCollision.hpp>
#include "../CoordSpaceHelper.hpp"
#include "../DrawComponents/DrawComponent.hpp"

class DrawingProgram;

class RectSelectTool {
    public:
        RectSelectTool(DrawingProgram& initDrawP);
        void gui_toolbox();
        void tool_update();
        void draw(SkCanvas* canvas, const DrawData& drawData);
        void reset_tool();
        void paste_clipboard();
    private:
        struct RectSelectControls {
            int selectionMode = 0;
            Vector2f selectStartAt;
            CoordSpaceHelper coords;

            WorldVec moveStartAt;
            SCollision::AABB<WorldScalar> finalSelectionAABB;
            std::array<Vector2f, 4> newT;

            WorldVec scaleStartAt;
            WorldVec scaleCenterPoint;
            WorldVec scaleCurrentPoint;

            WorldVec rotationCenter;

            // Temporary, per frame colliders for handles
            Vector2f rotationPoint;
            SCollision::Circle<float> scaleSelectionCircle;
            SCollision::Circle<float> rotationCenterCircle;
            SCollision::Circle<float> rotationPointCircle;

            double rotationPointAngle;

            struct SelectedItemInfo {
                std::shared_ptr<DrawComponent> comp;
                ServerClientID id;
                CoordSpaceHelper oldCoords;
            };
            std::vector<SelectedItemInfo> selectedItems;
        } controls;

        void selection_to_clipboard();
        Vector2f get_rotation_point_pos_from_angle(double angle);
        void update_drag_circles();
        void set_selected_items();
        bool create_selection_aabb();
        void commit_transformation();

        DrawingProgram& drawP;
};
