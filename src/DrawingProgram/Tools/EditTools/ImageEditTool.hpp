#pragma once
#include "DrawingProgramEditToolBase.hpp"
#include <any>

class DrawingProgram;

class ImageEditTool : public DrawingProgramEditToolBase {
    public:
        ImageEditTool(DrawingProgram& initDrawP, CanvasComponentContainer::ObjInfo* initComp);
        virtual void edit_start(EditTool& editTool, std::any& prevData) override;
        virtual void commit_edit_updates(std::any& prevData) override;
        virtual bool edit_update() override;
        virtual void edit_gui(Toolbar& t) override;
};
