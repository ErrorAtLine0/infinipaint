#pragma once
#include "DrawingProgramCache.hpp"

class DrawingProgram;

class DrawingProgramSelection {
    public:
        DrawingProgramSelection(DrawingProgram& initDrawP);
        void add_from_cam_coord_collider_to_selection(const SCollision::ColliderCollection<float>& cC);
        void deselect_all();
        void update();
        void draw(SkCanvas* canvas, const DrawData& drawData);
        bool is_something_selected();
    private:
        void calculate_aabb();
        bool mouse_collided_with_selection_aabb();

        SCollision::AABB<WorldScalar> selectionAABB;
        CoordSpaceHelper selectionTransformCoords;
        std::unordered_set<CollabListType::ObjectInfoPtr> selectedSet;
        DrawingProgramCache cache;
        DrawingProgram& drawP;
};
