#pragma once
#include "../CoordSpaceHelperTransform.hpp"
#include "Layers/DrawingProgramLayerManager.hpp"
#include "DrawingProgramCache.hpp"

class DrawingProgram;

class DrawingProgramSelection {
    public:
        DrawingProgramSelection(DrawingProgram& initDrawP);
        void add_from_cam_coord_collider_to_selection(const SCollision::ColliderCollection<float>& cC, DrawingProgramLayerManager::LayerSelector layerSelector, bool frontObjectOnly);
        void remove_from_cam_coord_collider_to_selection(const SCollision::ColliderCollection<float>& cC, DrawingProgramLayerManager::LayerSelector layerSelector, bool frontObjectOnly);
        void erase_component(CanvasComponentContainer::ObjInfo* objToCheck);
        bool is_something_selected();
        bool is_selected(CanvasComponentContainer::ObjInfo* objToCheck);
        void draw_components(SkCanvas* canvas, const DrawData& drawData);
        void draw_gui(SkCanvas* canvas, const DrawData& drawData);
        bool is_being_transformed();
        void update();
        void deselect_all();
        void paste_clipboard(Vector2f pasteScreenPos);
        std::unordered_set<CanvasComponentContainer::ObjInfo*> get_selection_as_set();
        void delete_all();
        void selection_to_clipboard();
        CanvasComponentContainer::ObjInfo* get_front_object_colliding_with_in_editing_layer(const SCollision::ColliderCollection<float>& cC);
    private:
        bool mouse_collided_with_selection_aabb();
        bool mouse_collided_with_scale_point();
        bool mouse_collided_with_rotate_center_handle_point();
        bool mouse_collided_with_rotate_handle_point();

        bool is_empty_transform();

        void commit_transform_selection();
        void rebuild_cam_space();
        Vector2f get_rotation_point_pos_from_angle(double angle);

        void set_to_selection(const std::vector<CanvasComponentContainer::ObjInfo*>& newSelection);
        void add_to_selection(const std::vector<CanvasComponentContainer::ObjInfo*>& newSelection);
        void sort_selection();
        void calculate_aabb();
        void reset_all();
        void reset_transform_data();
        std::function<bool(const std::shared_ptr<DrawingProgramCacheBVHNode>&)> erase_select_objects_in_bvh_func(std::vector<CanvasComponentContainer::ObjInfo*>& selectedComponents, const SCollision::ColliderCollection<float>& cC, const SCollision::ColliderCollection<WorldScalar>& cCWorld, DrawingProgramLayerManager::LayerSelector layerSelector);

        SCollision::ColliderCollection<float> camSpaceSelection;
        std::array<WorldVec, 4> selectionRectPoints;
        CoordSpaceHelperTransform selectionTransformCoords;
        std::vector<CanvasComponentContainer::ObjInfo*> selectedSet;
        SCollision::AABB<WorldScalar> initialSelectionAABB;
        DrawingProgram& drawP;

        enum class TransformOperation {
            NONE = 0,
            TRANSLATE,
            SCALE,
            ROTATE_RELOCATE_CENTER,
            ROTATE
        } transformOpHappening = TransformOperation::NONE;

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
};
