#pragma once
#include "DrawingProgramEditToolBase.hpp"
#include <any>

class DrawingProgram;

class ImageEditTool : public DrawingProgramEditToolBase {
    public:
        ImageEditTool(DrawingProgram& initDrawP);
        virtual void edit_start(EditTool& editTool, CanvasComponentContainer::ObjInfo* comp, std::any& prevData) override;
        virtual void commit_edit_updates(CanvasComponentContainer::ObjInfo* comp, std::any& prevData) override;
        virtual bool edit_update(CanvasComponentContainer::ObjInfo* comp) override;
        virtual bool edit_gui(CanvasComponentContainer::ObjInfo* comp) override;
};
