#pragma once
#include "DrawingProgramToolBase.hpp"
#include <Helpers/SCollision.hpp>
#include <filesystem>
#include "../../InputManager.hpp"
#include "../../CoordSpaceHelper.hpp"

class DrawingProgram;

class ScreenshotTool : public DrawingProgramToolBase {
    public:
        ScreenshotTool(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox() override;
        virtual void right_click_popup_gui(Vector2f popupPos) override;
        virtual void erase_component(CanvasComponentContainer::ObjInfo* erasedComp) override;
        virtual void tool_update() override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
        virtual bool prevent_undo_or_redo() override;
        virtual void input_mouse_button_on_canvas_callback(const InputManager::MouseButtonCallbackArgs& button) override;
        virtual void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion) override;

        enum ScreenshotType : size_t {
            SCREENSHOT_JPG,
            SCREENSHOT_PNG,
            SCREENSHOT_WEBP,
            SCREENSHOT_SVG
        };
    private:
        void commit_rect();
        void take_screenshot(const std::filesystem::path& filePath, ScreenshotType screenshotType);
        void take_screenshot_area_hw(const sk_sp<SkSurface>& surface, SkCanvas* canvas, void* fullImgRawData, const Vector2i& fullImageSize, const Vector2i& sectionImagePos, const Vector2i& sectionImageSize, const Vector2i& canvasSize, bool transparentBackground);
        void take_screenshot_svg(SkCanvas* canvas, bool transparentBackground);

        struct ScreenshotControls {
            CoordSpaceHelper translateBeginCoords;
            WorldVec translateBeginPos;

            CoordSpaceHelper coords;
            float rectX1;
            float rectX2;
            float rectY1;
            float rectY2;
            enum class SelectionMode {
                NO_SELECTION,
                DRAGGING_BORDER,
                SELECTION_EXISTS,
                DRAGGING_AREA
            } selectionMode = SelectionMode::NO_SELECTION;
            int dragType = 0;
            Vector2i imageSize = {0, 0};
            std::array<SCollision::Circle<float>, 8> circles;

            bool displayGrid = true;
            bool transparentBackground = false;

            std::vector<std::string> typeSelections = {".jpg", ".png", ".webp", ".svg"};

            std::atomic<bool> setToTakeScreenshot = false;
            std::filesystem::path screenshotSavePath;
            ScreenshotType screenshotSaveType;
        } controls;
        bool dragging_border_update(const Vector2f& camCursorPos);
        bool selection_exists_update();
        bool dragging_area_update(const Vector2f& camCursorPos);
};

NLOHMANN_JSON_SERIALIZE_ENUM(ScreenshotTool::ScreenshotType, {
    {ScreenshotTool::ScreenshotType::SCREENSHOT_JPG, "jpg"},
    {ScreenshotTool::ScreenshotType::SCREENSHOT_PNG, "png"},
    {ScreenshotTool::ScreenshotType::SCREENSHOT_WEBP, "webp"},
    {ScreenshotTool::ScreenshotType::SCREENSHOT_SVG, "svg"}
})
