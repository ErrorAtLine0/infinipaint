#pragma once
#include "DrawingProgramEditToolBase.hpp"
#include "../DrawComponents/DrawEllipse.hpp"
#include <any>
#include <Helpers/SCollision.hpp>

class DrawingProgram;

class EllipseDrawEditTool : public DrawingProgramEditToolBase {
    public:
        EllipseDrawEditTool(DrawingProgram& initDrawP);
        virtual void edit_start(EditTool& editTool, const std::shared_ptr<DrawComponent>& comp, std::any& prevData) override;
        virtual void commit_edit_updates(const std::shared_ptr<DrawComponent>& comp, std::any& prevData) override;
        virtual bool edit_update(const std::shared_ptr<DrawComponent>& comp) override;
        virtual bool edit_gui(const std::shared_ptr<DrawComponent>& comp) override;
    private:
        void commit();

        DrawEllipse::Data oldData;
};
