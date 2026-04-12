#pragma once
#include "DrawingProgramEditToolBase.hpp"
#include "../../../CanvasComponents/CanvasComponentContainer.hpp"
#include "../../../CanvasComponents//EllipseCanvasComponent.hpp"
#include <any>
#include <Helpers/SCollision.hpp>

class DrawingProgram;

class EllipseDrawEditTool : public DrawingProgramEditToolBase {
    public:
        EllipseDrawEditTool(DrawingProgram& initDrawP, CanvasComponentContainer::ObjInfo* initComp);
        virtual void edit_start(EditTool& editTool, std::any& prevData) override;
        virtual void commit_edit_updates(std::any& prevData) override;
        virtual bool edit_update() override;
        virtual void edit_gui(Toolbar& t) override;
    private:
        void commit();

        std::optional<EllipseCanvasComponent::Data> oldData;
};
