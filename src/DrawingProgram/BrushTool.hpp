#pragma once
#include "../SharedTypes.hpp"
#include <Helpers/Hashes.hpp>
#include <include/core/SkPath.h>
#include <include/core/SkVertices.h>
#include <Helpers/SCollision.hpp>
#include <deque>
#include <any>
#include <include/core/SkCanvas.h>

class DrawingProgram;
class DrawBrushStroke;
struct DrawData;

class BrushTool {
    public:
        BrushTool(DrawingProgram& initDrawP);
        void gui_toolbox();
        void tool_update();
        void reset_tool();
        void draw(SkCanvas* canvas, const DrawData& drawData);
    private:
        void commit_stroke();
        bool extensive_point_checking_back(const Vector2f& newPoint);
        bool extensive_point_checking(const Vector2f& newPoint);

        struct BrushToolControls {
            float penWidth = 1.0f;
            bool addedTemporaryPoint = false;
            struct SmoothingPoint {
                float val;
                std::chrono::steady_clock::time_point t;
            };
            std::deque<SmoothingPoint> velocitySmoothingData;
            std::deque<SmoothingPoint> penSmoothingData;

            std::shared_ptr<DrawBrushStroke> intermediateItem;
            bool isDrawing = false;
            bool hasRoundCaps = true;
            bool drawingMinimumRelativeToSize = true;
            bool midwayInterpolation = true;
            Vector2f prevPointUnaltered = {0, 0};
        } controls;

        DrawingProgram& drawP;
};
