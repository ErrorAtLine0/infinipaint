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
        bool mouse_collided_with_selection_aabb();
    private:
        void calculate_aabb();
        void rebuild_cam_space();

        SCollision::AABB<WorldScalar> initialSelectionAABB;
        SCollision::ColliderCollection<float> camSpaceSelection;

        WorldVec selectionRectMin;
        WorldVec selectionRectMax;

        CoordSpaceHelper selectionTransformCoords;
        std::unordered_set<CollabListType::ObjectInfoPtr> selectedSet;
        DrawingProgramCache cache;
        DrawingProgram& drawP;

        struct TranslationData {
            bool happening = false;
        } translation;
};
