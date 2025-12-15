#pragma once
#include "DrawingProgramEditToolBase.hpp"
#include "../CanvasComponents/RectangleCanvasComponent.hpp"
#include <Helpers/SCollision.hpp>
#include <any>

class DrawingProgram;

class RectDrawEditTool : public DrawingProgramEditToolBase {
    public:
        RectDrawEditTool(DrawingProgram& initDrawP);
        virtual void edit_start(EditTool& editTool, const CanvasComponentContainer::ObjInfoSharedPtr& comp, std::any& prevData) override;
        virtual void commit_edit_updates(const CanvasComponentContainer::ObjInfoSharedPtr& comp, std::any& prevData) override;
        virtual bool edit_update(const CanvasComponentContainer::ObjInfoSharedPtr& comp) override;
        virtual bool edit_gui(const CanvasComponentContainer::ObjInfoSharedPtr& comp) override;
    private:
        std::optional<RectangleCanvasComponent::Data> oldData;
};
