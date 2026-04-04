#pragma once
#include "DrawingProgramEditToolBase.hpp"
#include "../../../CanvasComponents/BrushStrokeCanvasComponent.hpp"
#include <Helpers/SCollision.hpp>
#include <any>

class DrawingProgram;

class BrushEditTool : public DrawingProgramEditToolBase {
    public:
        BrushEditTool(DrawingProgram& initDrawP, CanvasComponentContainer::ObjInfo* initComp);
        virtual void edit_start(EditTool& editTool, std::any& prevData) override;
        virtual void commit_edit_updates(std::any& prevData) override;
        virtual bool edit_update() override;
        virtual void edit_gui() override;
    private:
        Vector4f oldColor;
};
