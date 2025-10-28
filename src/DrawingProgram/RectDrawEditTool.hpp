#pragma once
#include "DrawingProgramEditToolBase.hpp"
#include "../DrawComponents/DrawRectangle.hpp"
#include <Helpers/SCollision.hpp>
#include <any>

class DrawingProgram;

class RectDrawEditTool : public DrawingProgramEditToolBase {
    public:
        RectDrawEditTool(DrawingProgram& initDrawP);
        virtual void edit_start(EditTool& editTool, const std::shared_ptr<DrawComponent>& comp, std::any& prevData) override;
        virtual void commit_edit_updates(const std::shared_ptr<DrawComponent>& comp, std::any& prevData) override;
        virtual bool edit_update(const std::shared_ptr<DrawComponent>& comp) override;
        virtual bool edit_gui(const std::shared_ptr<DrawComponent>& comp) override;
    private:
        std::optional<DrawRectangle::Data> oldData;
};
