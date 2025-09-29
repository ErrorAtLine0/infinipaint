#pragma once
#include "DrawingProgramCache.hpp"
#include "../CoordSpaceHelperTransform.hpp"

class DrawingProgram;

class DrawingProgramSelection {
    public:
        DrawingProgramSelection(DrawingProgram& initDrawP);
        void add_from_cam_coord_collider_to_selection(const SCollision::ColliderCollection<float>& cC, bool onlyFrontObject = false);
        void remove_from_cam_coord_collider_to_selection(const SCollision::ColliderCollection<float>& cC, bool onlyFrontObject = false);
        CollabListType::ObjectInfoPtr get_front_object_colliding_with(const SCollision::ColliderCollection<float>& cC);
        
        void deselect_all();
        void update();
        void draw_components(SkCanvas* canvas, const DrawData& drawData);
        void draw_gui(SkCanvas* canvas, const DrawData& drawData);
        bool is_something_selected();
        bool mouse_collided_with_selection();
        void erase_component(const CollabListType::ObjectInfoPtr& objToCheck);
        bool is_selected(const CollabListType::ObjectInfoPtr& objToCheck);
        const std::unordered_set<CollabListType::ObjectInfoPtr>& get_selected_set();

        bool is_being_transformed();

        void invalidate_cache_at_optional_aabb_before_pos(const std::optional<SCollision::AABB<WorldScalar>>& aabb, uint64_t placementToInvalidateAt);
        void clear_own_cached_surfaces();
        void preupdate_component(const CollabListType::ObjectInfoPtr& objToCheck);
    private:
        std::function<bool(const std::shared_ptr<DrawingProgramCacheBVHNode>&, std::vector<CollabListType::ObjectInfoPtr>&)> erase_select_objects_in_bvh_func(std::unordered_set<CollabListType::ObjectInfoPtr>& selectedComponents, const SCollision::ColliderCollection<float>& cC, const SCollision::ColliderCollection<WorldScalar>& cCWorld);
        void fully_collided_erase_select_objects_func(std::unordered_set<CollabListType::ObjectInfoPtr>& selectedComponents, const std::shared_ptr<DrawingProgramCacheBVHNode>& bvhNode);

        void delete_all();
        bool mouse_collided_with_selection_aabb();
        bool mouse_collided_with_scale_point();
        bool mouse_collided_with_rotate_center_handle_point();
        bool mouse_collided_with_rotate_handle_point();
        void selection_to_clipboard();
        void paste_clipboard();

        void set_to_selection(const std::unordered_set<CollabListType::ObjectInfoPtr>& newSelection);
        void add_to_selection(const std::unordered_set<CollabListType::ObjectInfoPtr>& newSelection);
        void add_erased_objects_back_to_cache(const std::unordered_set<CollabListType::ObjectInfoPtr>& erasedObjects);

        void commit_transform_selection();

        void calculate_aabb();
        void calculate_initial_rotate_center_location();
        void rebuild_cam_space();

        Vector2f get_rotation_point_pos_from_angle(double angle);

        SCollision::AABB<WorldScalar> initialSelectionAABB;
        SCollision::ColliderCollection<float> camSpaceSelection;

        std::array<WorldVec, 4> selectionRectPoints;

        CoordSpaceHelperTransform selectionTransformCoords;

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
