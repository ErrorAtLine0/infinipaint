#pragma once
#include "DrawingProgramToolBase.hpp"
#include <Helpers/Hashes.hpp>
#include <include/core/SkPath.h>
#include <include/core/SkVertices.h>
#include <Helpers/SCollision.hpp>
#include <deque>
#include <include/core/SkCanvas.h>
#include "../CanvasComponents/CanvasComponentContainer.hpp"
#include <Helpers/NetworkingObjects/NetObjWeakPtr.hpp>

class DrawingProgram;
struct DrawData;
class BrushStrokeCanvasComponent;

class BrushTool : public DrawingProgramToolBase {
    public:
        BrushTool(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() override;
        virtual void gui_toolbox() override;
        virtual void tool_update() override;
        virtual bool right_click_popup_gui(Vector2f popupPos) override;
        virtual void erase_component(const CanvasComponentContainer::ObjInfoSharedPtr& erasedComp) override;
        virtual void switch_tool(DrawingProgramToolType newTool) override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual bool prevent_undo_or_redo() override;
    private:
        bool extensive_point_checking_back(const BrushStrokeCanvasComponent& brushStroke, const Vector2f& newPoint);
        bool extensive_point_checking(const BrushStrokeCanvasComponent& brushStroke, const Vector2f& newPoint);
        void commit_stroke();

        float penWidth = 1.0f;
        bool addedTemporaryPoint = false;
        struct SmoothingPoint {
            float val;
            std::chrono::steady_clock::time_point t;
        };
        std::deque<SmoothingPoint> penSmoothingData;

        CanvasComponentContainer::ObjInfoSharedPtr objInfoBeingEdited;
        bool isDrawing = false;
        bool hasRoundCaps = true;
        bool drawingMinimumRelativeToSize = true;
        bool midwayInterpolation = true;
        Vector2f prevPointUnaltered = {0, 0};
};
