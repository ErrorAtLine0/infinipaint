#pragma once
#include "DrawingProgramEditToolBase.hpp"
#include "../../../CanvasComponents/BrushStrokeCanvasComponent.hpp"
#include <Helpers/SCollision.hpp>
#include <any>

class DrawingProgram;

class BrushEditTool : public DrawingProgramEditToolBase {
    public:
        BrushEditTool(DrawingProgram& initDrawP);
        virtual void edit_start(EditTool& editTool, CanvasComponentContainer::ObjInfo* comp, std::any& prevData) override;
        virtual void commit_edit_updates(CanvasComponentContainer::ObjInfo* comp, std::any& prevData) override;
        virtual bool edit_update(CanvasComponentContainer::ObjInfo* comp) override;
        virtual bool edit_gui(CanvasComponentContainer::ObjInfo* comp) override;
    private:
        Vector4f oldColor;
};
