#pragma once
#include "DrawingProgramToolBase.hpp"
#include <Helpers/SCollision.hpp>
#include <filesystem>
#include "../InputManager.hpp"
#include "../CoordSpaceHelper.hpp"

class DrawingProgram;

class ScreenshotTool : public DrawingProgramToolBase {
    public:
        ScreenshotTool(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox() override;
        virtual void tool_update() override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
        virtual bool prevent_undo_or_redo() override;
    private:
        void commit_rect();
        void take_screenshot(const std::filesystem::path& filePath);
        void take_screenshot_area_hw(const sk_sp<SkSurface>& surface, SkCanvas* canvas, void* fullImgRawData, const Vector2i& fullImageSize, const Vector2i& sectionImagePos, const Vector2i& sectionImageSize, const Vector2i& canvasSize, bool transparentBackground);

        //static constexpr std::array<InputManager::SystemCursorType, 8> cursors{
        //    InputManager::SystemCursorType::SE_RESIZE,
        //    InputManager::SystemCursorType::S_RESIZE,
        //    InputManager::SystemCursorType::SW_RESIZE,
        //    InputManager::SystemCursorType::W_RESIZE,
        //    InputManager::SystemCursorType::NW_RESIZE,
        //    InputManager::SystemCursorType::N_RESIZE,
        //    InputManager::SystemCursorType::NE_RESIZE,
        //    InputManager::SystemCursorType::E_RESIZE
        //};

        struct ScreenshotControls {
            CoordSpaceHelper coords;
            float rectX1;
            float rectX2;
            float rectY1;
            float rectY2;
            int selectionMode = 0;
            int dragType = 0;
            Vector2i imageSize = {0, 0};
            std::array<SCollision::Circle<float>, 8> circles;

            bool displayGrid = true;
            bool transparentBackground = false;

            size_t selectedType = 0;
            std::vector<std::string> typeSelections = {".jpg", ".png", ".webp"};

            std::atomic<bool> setToTakeScreenshot = false;
            std::filesystem::path screenshotSavePath;
        } controls;
};
