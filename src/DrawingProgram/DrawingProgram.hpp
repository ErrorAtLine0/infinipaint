#pragma once
#include "../DrawData.hpp"
#include "DrawingProgramCache.hpp"
#include "cereal/archives/portable_binary.hpp"
#include <include/core/SkCanvas.h>
#include <include/core/SkPath.h>
#include <include/core/SkVertices.h>
#include <Helpers/SCollision.hpp>
#include <Helpers/Hashes.hpp>
#include <list>
#include <Helpers/Random.hpp>
#include "BrushTool.hpp"
#include "EraserTool.hpp"
#include "RectSelectTool.hpp"
#include "RectDrawTool.hpp"
#include "EllipseDrawTool.hpp"
#include "TextBoxTool.hpp"
#include "InkDropperTool.hpp"
#include "ScreenshotTool.hpp"
#include "EditTool.hpp"
#include "ImageTool.hpp"
#include "DrawingProgramSelection.hpp"
#include "../CollabList.hpp"

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

        DrawingProgramCache compCache;

        CollabListType components;

        std::unordered_set<std::shared_ptr<DrawComponent>> updateableComponents;
        std::unordered_map<std::shared_ptr<DrawComponent>, std::shared_ptr<DrawComponent>> delayedUpdateComponents;

        Vector4f* get_foreground_color_ptr();
    private:
        void client_erase_set(std::unordered_set<CollabListType::ObjectInfoPtr> erasedComponents); // The set might be modified while this is being called, so dont pass by reference

        void check_delayed_update_timers();
        void check_updateable_components();

        void force_rebuild_cache();

        bool addToCompCacheOnInsert = true;

        std::atomic<bool> addFileInNextFrame = false;
        std::pair<std::string, Vector2f> addFileInfo;

        void add_file_to_canvas_by_path_execute(const std::string& filePath, Vector2f dropPos);

        void reset_tools();

        void drag_drop_update();

        void draw_drag_circle(SkCanvas* canvas, const Vector2f& pos, const SkColor4f& c, const DrawData& drawData, float radiusMultiplier = 1.0f);

        void add_undo_place_component(const CollabListType::ObjectInfoPtr& objToUndo);
        void add_undo_place_components(const std::unordered_set<CollabListType::ObjectInfoPtr>& objSetToUndo);
        void add_undo_erase_components(const std::unordered_set<CollabListType::ObjectInfoPtr>& objSetToUndo);

        BrushTool brushTool;
        EraserTool eraserTool;
        RectSelectTool rectSelectTool;
        RectDrawTool rectDrawTool;
        EllipseDrawTool ellipseDrawTool;
        TextBoxTool textBoxTool;
        InkDropperTool inkDropperTool;
        ScreenshotTool screenshotTool;
        EditTool editTool;
        ImageTool imageTool;

        DrawingProgramSelection selection;

        bool temporaryEraser = false;

        enum Tool : int {
            TOOL_BRUSH = 0,
            TOOL_ERASER,
            TOOL_RECTSELECT,
            TOOL_RECTANGLE,
            TOOL_ELLIPSE,
            TOOL_TEXTBOX,
            TOOL_INKDROPPER,
            TOOL_SCREENSHOT,
            TOOL_EDIT
        };

        struct GlobalControls {
            float relativeWidth = 15.0f;

            Vector4f foregroundColor{0.9f, 0.9f, 0.9f, 1.0f};
            Vector4f backgroundColor{0.0f, 0.0f, 0.0f, 1.0f};
            int previousSelected = 0;
            WorldVec previousMouseWorldPos = {0, 0};
            WorldVec currentMouseWorldPos = {0, 0};
            int selectedTool = 0;
            bool leftClick = false;
            bool leftClickHeld = false;
            bool leftClickReleased = false;
            bool cursorHoveringOverCanvas = false;

            int colorEditing = 0;
        } controls;

        uint32_t nextID = 0;
        
        friend class InkDropperTool;
        friend class RectDrawTool;
        friend class EllipseDrawTool;
        friend class BrushTool;
        friend class EraserTool;
        friend class RectSelectTool;
        friend class TextBoxTool;
        friend class ScreenshotTool;
        friend class EditTool;
        friend class ImageTool;
        friend class DrawingProgramCache;
        friend class DrawingProgramSelection;
};
