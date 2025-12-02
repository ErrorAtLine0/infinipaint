#pragma once
#include <include/core/SkCanvas.h>
#include "../DrawData.hpp"
#include "../DrawComponents/DrawComponent.hpp"

class DrawingProgram;
class EditTool;

class DrawingProgramEditToolBase {
    public:
        DrawingProgramEditToolBase(DrawingProgram& initDrawP);
        virtual void edit_start(EditTool& editTool, const std::shared_ptr<DrawComponent>& a, std::any& prevData) = 0;
        virtual void commit_edit_updates(const std::shared_ptr<DrawComponent>& a, std::any& prevData) = 0;
        virtual bool edit_update(const std::shared_ptr<DrawComponent>& a) = 0;
        virtual bool edit_gui(const std::shared_ptr<DrawComponent>& a) = 0;
        virtual bool right_click_popup_gui(const std::shared_ptr<DrawComponent>& comp, Vector2f popupPos);
        virtual ~DrawingProgramEditToolBase(); 
    protected:
        DrawingProgram& drawP;
};
