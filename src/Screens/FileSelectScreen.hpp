#pragma once
#include "Screen.hpp"

class FileSelectScreen : public Screen {
    public:
        FileSelectScreen(MainProgram& m);
        virtual void gui_layout_run() override;
        virtual void draw(SkCanvas* canvas) override;
};
