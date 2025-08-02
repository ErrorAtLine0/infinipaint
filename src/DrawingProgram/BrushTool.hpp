#pragma once
#include "../SharedTypes.hpp"
#include "DrawingProgramToolBase.hpp"
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

class BrushTool : public DrawingProgramToolBase {
    public:
        BrushTool(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox() override;
        virtual void tool_update() override;
        virtual void reset_tool() override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual bool prevent_undo_or_redo() override;
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
};
