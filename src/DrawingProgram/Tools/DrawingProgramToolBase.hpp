#pragma once
#include <include/core/SkCanvas.h>
#include "../../DrawData.hpp"
#include "../../CanvasComponents/CanvasComponentContainer.hpp"

class DrawingProgram;

#define MINIMUM_DISTANCE_BETWEEN_BOUNDS 10.0f

enum class DrawingProgramToolType : int {
    BRUSH = 0,
    ERASER,
    LASSOSELECT,
    RECTSELECT,
    RECTANGLE,
    ELLIPSE,
    TEXTBOX,
    EYEDROPPER,
    SCREENSHOT,
    GRIDMODIFY,
    EDIT,
    ZOOM,
    PAN
};

class DrawingProgramToolBase {
    public:
        DrawingProgramToolBase(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() = 0;
        virtual void gui_toolbox() = 0;
        virtual void erase_component(const CanvasComponentContainer::ObjInfoSharedPtr& erasedComp) = 0;
        virtual bool right_click_popup_gui(Vector2f popupPos) = 0;
        virtual void tool_update() = 0;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) = 0;
        virtual void switch_tool(DrawingProgramToolType newTool) = 0;
        virtual bool prevent_undo_or_redo() = 0;
        virtual ~DrawingProgramToolBase(); 
        static std::unique_ptr<DrawingProgramToolBase> allocate_tool_type(DrawingProgram& drawP, DrawingProgramToolType t);
    protected:
        DrawingProgram& drawP;
};
