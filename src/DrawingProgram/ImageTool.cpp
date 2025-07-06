#include "ImageTool.hpp"
#include "DrawingProgram.hpp"
#include "../World.hpp"
#include "../Toolbar.hpp"
#include "../MainProgram.hpp"
#include "../ResourceManager.hpp"
#include <fstream>
#ifdef __EMSCRIPTEN__
    #include <EmscriptenHelpers/emscripten_browser_file.h>
#endif

ImageTool::ImageTool(DrawingProgram& initDrawP):
    drawP(initDrawP)
{}

void ImageTool::edit_start(const std::shared_ptr<DrawImage>& a, std::any& prevData) {
}

void ImageTool::commit_edit_updates(const std::shared_ptr<DrawImage>& a, std::any& prevData) {
}

bool ImageTool::edit_update(const std::shared_ptr<DrawImage>& a) {
    return true;
}

bool ImageTool::edit_gui(const std::shared_ptr<DrawImage>& a) {
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("edit tool image");
    t.gui.text_label_centered("File Properties");
    std::optional<ResourceData> resourceData = drawP.world.rMan.get_resource(a->d.imageID);
    if(resourceData) {
        t.gui.text_label("Name: " + resourceData->name);
        if(t.gui.text_button_wide("file download", "Download file")) {
            #ifdef __EMSCRIPTEN__
                emscripten_browser_file::download(
                    resourceData->name,
                    "application/octet-binary",
                    *resourceData->data
                );
            #endif
            t.open_file_selector("Download File", {{"Any File", "*"}}, [resourceData](const std::filesystem::path& p, const auto& e) {
                std::ofstream f(p, std::ios::binary);
                if(f.is_open()) {
                    f << *resourceData->data;
                    f.close();
                }
            }, resourceData->name, true);
        }
    }
    else
        t.gui.text_label_centered("Loading resource...");
    t.gui.pop_id();
    return false;
}
