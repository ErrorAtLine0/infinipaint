#pragma once
#include <include/core/SkCanvas.h>
#include "../DrawData.hpp"
#include "../CanvasComponents/CanvasComponentContainer.hpp"

class DrawingProgram;
class EditTool;

class DrawingProgramEditToolBase {
    public:
        DrawingProgramEditToolBase(DrawingProgram& initDrawP);
        virtual void edit_start(EditTool& editTool, const CanvasComponentContainer::ObjInfoSharedPtr& compContainer, std::any& prevData) = 0;
        virtual void commit_edit_updates(const CanvasComponentContainer::ObjInfoSharedPtr& compContainer, std::any& prevData) = 0;
        virtual bool edit_update(const CanvasComponentContainer::ObjInfoSharedPtr& compContainer) = 0;
        virtual bool edit_gui(const CanvasComponentContainer::ObjInfoSharedPtr& compContainer) = 0;
        virtual bool right_click_popup_gui(const CanvasComponentContainer::ObjInfoSharedPtr& comp, Vector2f popupPos);
        virtual ~DrawingProgramEditToolBase(); 
    protected:
        DrawingProgram& drawP;
};
