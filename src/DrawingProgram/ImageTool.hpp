#pragma once
#include <include/core/SkCanvas.h>
#include "../DrawData.hpp"
#include "../DrawComponents/DrawRectangle.hpp"
#include "../DrawComponents/DrawImage.hpp"
#include <Helpers/SCollision.hpp>
#include <any>

class DrawingProgram;

class ImageTool {
    public:
        ImageTool(DrawingProgram& initDrawP);
        void edit_start(const std::shared_ptr<DrawImage>& a, std::any& prevData);
        void commit_edit_updates(const std::shared_ptr<DrawImage>& a, std::any& prevData);
        bool edit_update(const std::shared_ptr<DrawImage>& a);
        bool edit_gui(const std::shared_ptr<DrawImage>& a);
    private:
        struct ImageControls {
        } controls;

        DrawingProgram& drawP;
};
