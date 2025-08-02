#pragma once
#include <include/core/SkCanvas.h>
#include "../DrawData.hpp"

class DrawingProgram;

enum class DrawingProgramToolType : int {
    BRUSH = 0,
    ERASER,
    LASSOSELECT,
    RECTSELECT,
    RECTANGLE,
    ELLIPSE,
    TEXTBOX,
    INKDROPPER,
    SCREENSHOT,
    EDIT
};

class DrawingProgramToolBase {
    public:
        DrawingProgramToolBase(DrawingProgram& initDrawP);
        virtual DrawingProgramToolType get_type() = 0;
        virtual void gui_toolbox() = 0;
        virtual void tool_update() = 0;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) = 0;
        virtual void reset_tool() = 0;
        virtual bool prevent_undo_or_redo() = 0;
        virtual ~DrawingProgramToolBase(); 
        static std::unique_ptr<DrawingProgramToolBase> allocate_tool_type(DrawingProgram& drawP, DrawingProgramToolType t);
    protected:
        DrawingProgram& drawP;
};
