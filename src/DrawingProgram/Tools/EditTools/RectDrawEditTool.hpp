#pragma once
#include "DrawingProgramEditToolBase.hpp"
#include "../../../CanvasComponents/RectangleCanvasComponent.hpp"
#include <Helpers/SCollision.hpp>
#include <any>

class DrawingProgram;

class RectDrawEditTool : public DrawingProgramEditToolBase {
    public:
        RectDrawEditTool(DrawingProgram& initDrawP);
        virtual void edit_start(EditTool& editTool, CanvasComponentContainer::ObjInfo* comp, std::any& prevData) override;
        virtual void commit_edit_updates(CanvasComponentContainer::ObjInfo* comp, std::any& prevData) override;
        virtual bool edit_update(CanvasComponentContainer::ObjInfo* comp) override;
        virtual bool edit_gui(CanvasComponentContainer::ObjInfo* comp) override;
    private:
        std::optional<RectangleCanvasComponent::Data> oldData;
};
