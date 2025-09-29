#pragma once
#include "../DrawData.hpp"
#include "DrawingProgramCache.hpp"
#include "GridModifyTool.hpp"
#include "PanCanvasTool.hpp"
#include "ZoomCanvasTool.hpp"
#include "cereal/archives/portable_binary.hpp"
#include <include/core/SkCanvas.h>
#include <include/core/SkPath.h>
#include <include/core/SkVertices.h>
#include <Helpers/SCollision.hpp>
#include <Helpers/Hashes.hpp>
#include <list>
#include <Helpers/Random.hpp>
#include "DrawingProgramSelection.hpp"
#include "../CollabList.hpp"
#include "DrawingProgramToolBase.hpp"

class World;

#define DRAG_POINT_RADIUS 10.0f

class DrawingProgram {
    public:
        DrawingProgram(World& initWorld);
        void init_client_callbacks();
        void toolbar_gui();
        void tool_options_gui();
        void update();
        void draw(SkCanvas* canvas, const DrawData& drawData);
        void initialize_draw_data(cereal::PortableBinaryInputArchive& a);
        std::unordered_set<ServerClientID> get_used_resources();
        ClientPortionID get_max_id(ServerPortionID serverID);
        void write_to_file(cereal::PortableBinaryOutputArchive& a);
        World& world;

        bool prevent_undo_or_redo();

        void add_file_to_canvas_by_path(const std::string& filePath, Vector2f dropPos, bool addInSameThread);
        void add_file_to_canvas_by_data(const std::string& fileName, std::string_view fileBuffer, Vector2f dropPos);

        void parallel_loop_all_components(std::function<void(const std::shared_ptr<CollabList<std::shared_ptr<DrawComponent>, ServerClientID>::ObjectInfo>&)> func);

        // This function, unlike preupdate_component, does not take the object out of the BVH. Use for a draw update that doesn't change the object's bounding box
        void invalidate_cache_at_component(const CollabListType::ObjectInfoPtr& objToCheck);
        void preupdate_component(const CollabListType::ObjectInfoPtr& objToCheck);

        DrawingProgramCache compCache;

        CollabListType components;

        std::unordered_set<std::shared_ptr<DrawComponent>> updateableComponents;
        std::unordered_map<std::shared_ptr<DrawComponent>, std::shared_ptr<DrawComponent>> delayedUpdateComponents;

        Vector4f* get_foreground_color_ptr();

        void switch_to_tool(DrawingProgramToolType newToolType, bool force = false);
        void switch_to_tool_ptr(std::unique_ptr<DrawingProgramToolBase> newTool);

        void clear_draw_cache();

        void modify_grid(ServerClientID gridToModifyID);

    private:
        std::optional<std::chrono::steady_clock::time_point> badFrametimeTimePoint;
        std::optional<std::chrono::steady_clock::time_point> unorderedObjectsExistTimePoint;

        void client_erase_set(std::unordered_set<CollabListType::ObjectInfoPtr> erasedComponents); // The set might be modified while this is being called, so dont pass by reference

        void check_delayed_update_timers();
        void check_updateable_components();

        void force_rebuild_cache();

        bool addToCompCacheOnInsert = true;

        std::atomic<bool> addFileInNextFrame = false;
        std::pair<std::string, Vector2f> addFileInfo;

        void add_file_to_canvas_by_path_execute(const std::string& filePath, Vector2f dropPos);

        void drag_drop_update();

        void draw_drag_circle(SkCanvas* canvas, const Vector2f& pos, const SkColor4f& c, const DrawData& drawData, float radiusMultiplier = 1.0f);

        void add_undo_place_component(const CollabListType::ObjectInfoPtr& objToUndo);
        void add_undo_place_components(const std::unordered_set<CollabListType::ObjectInfoPtr>& objSetToUndo);
        void add_undo_erase_components(const std::unordered_set<CollabListType::ObjectInfoPtr>& objSetToUndo);

        SkPaint select_tool_line_paint();

        DrawingProgramSelection selection;

        bool temporaryEraser = false;
        bool temporaryPan = false;
        DrawingProgramToolType toolTypeAfterTempPan;

        bool is_selection_tool(DrawingProgramToolType typeToCheck);

        struct GlobalControls {
            float relativeWidth = 15.0f;

            Vector4f foregroundColor{1.0f, 1.0f, 1.0f, 1.0f};
            Vector4f backgroundColor{0.0f, 0.0f, 0.0f, 1.0f};
            WorldVec previousMouseWorldPos = {0, 0};
            WorldVec currentMouseWorldPos = {0, 0};
            bool leftClick = false;
            bool leftClickHeld = false;
            bool leftClickReleased = false;
            bool middleClick = false;
            bool middleClickHeld = false;
            bool middleClickReleased = false;
            bool cursorHoveringOverCanvas = false;

            int colorEditing = 0;
        } controls;

        std::unique_ptr<DrawingProgramToolBase> drawTool;

        uint32_t nextID = 0;
        
        friend class EyeDropperTool;
        friend class RectDrawTool;
        friend class EllipseDrawTool;
        friend class BrushTool;
        friend class EraserTool;
        friend class RectSelectTool;
        friend class TextBoxTool;
        friend class ScreenshotTool;
        friend class EditTool;
        friend class ImageEditTool;
        friend class RectDrawEditTool;
        friend class EllipseDrawEditTool;
        friend class TextBoxEditTool;
        friend class DrawingProgramCache;
        friend class DrawingProgramSelection;
        friend class LassoSelectTool;
        friend class GridEditTool;
        friend class GridModifyTool;
        friend class ZoomCanvasTool;
        friend class PanCanvasTool;
        friend class DrawCamera;
};
