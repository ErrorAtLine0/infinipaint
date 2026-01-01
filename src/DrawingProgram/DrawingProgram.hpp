#pragma once
#include "../DrawData.hpp"
#include "DrawingProgramCache.hpp"
#include "Tools/GridModifyTool.hpp"
#include <Helpers/NetworkingObjects/NetObjWeakPtr.hpp>
#include "Tools/PanCanvasTool.hpp"
#include "Tools/ZoomCanvasTool.hpp"
#include "cereal/archives/portable_binary.hpp"
#include <include/core/SkCanvas.h>
#include <include/core/SkPath.h>
#include <include/core/SkVertices.h>
#include <Helpers/SCollision.hpp>
#include <Helpers/Hashes.hpp>
#include <Helpers/Random.hpp>
#include "Tools/DrawingProgramToolBase.hpp"
#include <Helpers/FileDownloader.hpp>
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include "Layers/DrawingProgramLayerManager.hpp"
#include "DrawingProgramSelection.hpp"

class World;

class DrawingProgram {
    public:
        DrawingProgram(World& initWorld);
        void server_init_no_file();
        void toolbar_gui();
        void tool_options_gui();
        bool right_click_popup_gui(Vector2f popupPos);
        void update();
        void scale_up(const WorldScalar& scaleUpAmount);
        void draw(SkCanvas* canvas, const DrawData& drawData);
        void write_components_server(cereal::PortableBinaryOutputArchive& a);
        void read_components_client(cereal::PortableBinaryInputArchive& a);
        void init_server_callbacks();
        void init_client_callbacks();
        void add_file_to_canvas_by_path(const std::filesystem::path& filePath, Vector2f dropPos, bool addInSameThread);
        void add_file_to_canvas_by_data(const std::string& fileName, std::string_view fileBuffer, Vector2f dropPos);
        void get_used_resources(std::unordered_set<NetworkingObjects::NetObjID>& resourceSet);

        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version);
        void save_file(cereal::PortableBinaryOutputArchive& a) const;
        World& world;

        bool prevent_undo_or_redo();

        DrawingProgramCache drawCache;
        DrawingProgramLayerManager layerMan;

        Vector4f* get_foreground_color_ptr();
        void switch_to_tool(DrawingProgramToolType newToolType, bool force = false);
        void switch_to_tool_ptr(std::unique_ptr<DrawingProgramToolBase> newTool);
        void modify_grid(const NetworkingObjects::NetObjWeakPtr<WorldGrid>& gridToModify);

        void invalidate_cache_at_component(CanvasComponentContainer::ObjInfo* objToCheck);
        void preupdate_component(CanvasComponentContainer::ObjInfo* objToCheck);
        void send_transforms_for(const std::vector<CanvasComponentContainer::ObjInfo*>& objsToSendTransformsFor);
    private:
        void process_transform_message(const std::vector<std::pair<NetworkingObjects::NetObjID, CoordSpaceHelper>>& transforms);

        void drag_drop_update();
        void add_file_to_canvas_by_path_execute(const std::filesystem::path& filePath, Vector2f dropPos);
        void check_updateable_components();
        void update_downloading_dropped_files();

        std::optional<std::chrono::steady_clock::time_point> badFrametimeTimePoint;
        std::optional<std::chrono::steady_clock::time_point> unorderedObjectsExistTimePoint;

        bool selection_action_menu(Vector2f popupPos);
        void rebuild_cache();

        std::atomic<bool> addFileInNextFrame = false;
        std::pair<std::filesystem::path, Vector2f> addFileInfo;

        float drag_point_radius();
        void draw_drag_circle(SkCanvas* canvas, const Vector2f& pos, const SkColor4f& c, const DrawData& drawData, float radiusMultiplier = 1.0f);
        SkPaint select_tool_line_paint();
        bool is_actual_selection_tool(DrawingProgramToolType typeToCheck);
        bool is_selection_allowing_tool(DrawingProgramToolType typeToCheck);

        DrawingProgramSelection selection;

        std::unique_ptr<DrawingProgramToolBase> toolToSwitchToAfterUpdate;
        std::unordered_set<CanvasComponentContainer::ObjInfo*> updateableComponents;

        bool temporaryEraser = false;
        bool temporaryPan = false;
        DrawingProgramToolType toolTypeAfterTempPan;

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

            DrawingProgramLayerManager::LayerSelector layerSelector = DrawingProgramLayerManager::LayerSelector::LAYER_BEING_EDITED;

            int colorEditing = 0;
        } controls;

        struct DroppedDownloadingFile {
            CanvasComponentContainer::ObjInfo* comp;
            Vector2f windowSizeWhenDropped;
            std::shared_ptr<FileDownloader::DownloadData> downData;
        };
        std::vector<DroppedDownloadingFile> droppedDownloadingFiles;

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
        friend class DrawingProgramLayerManager;
        friend class DrawingProgramLayer;
        friend class DrawingProgramLayerFolder;
};
