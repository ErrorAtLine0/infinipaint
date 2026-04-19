#pragma once
#include "Screen.hpp"

class FileSelectScreen : public Screen {
    public:
        FileSelectScreen(MainProgram& m);
        virtual void gui_layout_run() override;
        virtual void draw(SkCanvas* canvas) override;
    private:
        const std::vector<std::string>& get_file_list();
        std::optional<std::vector<std::string>> fileListOptional;
};
