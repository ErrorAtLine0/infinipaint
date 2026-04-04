#pragma once
#include "DrawingProgramEditToolBase.hpp"
#include "../../../CanvasComponents/RectangleCanvasComponent.hpp"
#include <Helpers/SCollision.hpp>
#include <any>

class DrawingProgram;

class RectDrawEditTool : public DrawingProgramEditToolBase {
    public:
        RectDrawEditTool(DrawingProgram& initDrawP, CanvasComponentContainer::ObjInfo* initComp);
        virtual void edit_start(EditTool& editTool, std::any& prevData) override;
        virtual void commit_edit_updates(std::any& prevData) override;
        virtual bool edit_update() override;
        virtual void edit_gui() override;
    private:
        std::optional<RectangleCanvasComponent::Data> oldData;
};
