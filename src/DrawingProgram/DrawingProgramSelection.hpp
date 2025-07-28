#pragma once
#include "DrawingProgramCache.hpp"

class DrawingProgram;

class DrawingProgramSelection {
    public:
        DrawingProgramSelection(DrawingProgram& initDrawP);
        void add_from_cam_coord_collider_to_selection(const SCollision::ColliderCollection<float>& cC);
        void deselect_all();
        void update();
        void draw_components(SkCanvas* canvas, const DrawData& drawData);
        void draw_gui(SkCanvas* canvas, const DrawData& drawData);
        bool is_something_selected();
        bool mouse_collided_with_selection();
        void erase_component(const CollabListType::ObjectInfoPtr& objToCheck);
        bool is_selected(const CollabListType::ObjectInfoPtr& objToCheck);
        const std::unordered_set<CollabListType::ObjectInfoPtr>& get_selected_set();
    private:
        void delete_all();
        bool mouse_collided_with_selection_aabb();
        bool mouse_collided_with_scale_point();
        bool mouse_collided_with_rotate_center_handle_point();
        bool mouse_collided_with_rotate_handle_point();

        void calculate_aabb();
        void calculate_initial_rotate_center_location();
        void rebuild_cam_space();

        Vector2f get_rotation_point_pos_from_angle(double angle);

        SCollision::AABB<WorldScalar> initialSelectionAABB;
        SCollision::ColliderCollection<float> camSpaceSelection;

        std::array<WorldVec, 4> selectionRectPoints;

        CoordSpaceHelper selectionTransformCoords;
        std::unordered_set<CollabListType::ObjectInfoPtr> selectedSet;
        DrawingProgramCache cache;
        DrawingProgram& drawP;

        enum class TransformOperation {
            NONE = 0,
            TRANSLATE,
            SCALE,
            ROTATE_RELOCATE_CENTER,
            ROTATE
        } transformOpHappening = TransformOperation::NONE;

        CoordSpaceHelper startingSelectionTransformCoords;

        struct TranslationData {
            WorldVec startPos;
        } translateData;

        struct ScaleData {
            WorldVec startPos;
            WorldVec centerPos;
            WorldVec currentPos;
            Vector2f handlePoint;
        } scaleData;

        struct RotationData {
            WorldVec centerPos;
            Vector2f centerHandlePoint;
            Vector2f handlePoint;
            double rotationAngle = 0.0;
        } rotateData;

        void reset_selection_data();
        void reset_transform_data();
};
