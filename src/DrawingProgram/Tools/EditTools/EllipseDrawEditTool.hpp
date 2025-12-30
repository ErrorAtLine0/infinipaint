#pragma once
#include "DrawingProgramEditToolBase.hpp"
#include "../../../CanvasComponents/CanvasComponentContainer.hpp"
#include "../../../CanvasComponents//EllipseCanvasComponent.hpp"
#include <any>
#include <Helpers/SCollision.hpp>

class DrawingProgram;

class EllipseDrawEditTool : public DrawingProgramEditToolBase {
    public:
        EllipseDrawEditTool(DrawingProgram& initDrawP);
        virtual void edit_start(EditTool& editTool, CanvasComponentContainer::ObjInfo* comp, std::any& prevData) override;
        virtual void commit_edit_updates(CanvasComponentContainer::ObjInfo* comp, std::any& prevData) override;
        virtual bool edit_update(CanvasComponentContainer::ObjInfo* comp) override;
        virtual bool edit_gui(CanvasComponentContainer::ObjInfo* comp) override;
    private:
        void commit();

        std::optional<EllipseCanvasComponent::Data> oldData;
};
