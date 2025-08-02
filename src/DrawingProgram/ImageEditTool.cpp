#include "ImageEditTool.hpp"
#include "DrawingProgram.hpp"
#include "../DrawComponents/DrawImage.hpp"
#include "../World.hpp"
#include "../Toolbar.hpp"
#include "../MainProgram.hpp"
#include "../ResourceManager.hpp"
#include "DrawingProgramEditToolBase.hpp"
#include <fstream>
#ifdef __EMSCRIPTEN__
    #include <EmscriptenHelpers/emscripten_browser_file.h>
#endif

ImageEditTool::ImageEditTool(DrawingProgram& initDrawP):
    DrawingProgramEditToolBase(initDrawP)
{}

void ImageEditTool::edit_start(EditTool& editTool, const std::shared_ptr<DrawComponent>& comp, std::any& prevData) {
}

void ImageEditTool::commit_edit_updates(const std::shared_ptr<DrawComponent>& comp, std::any& prevData) {
}

bool ImageEditTool::edit_update(const std::shared_ptr<DrawComponent>& comp) {
    return true;
}

bool ImageEditTool::edit_gui(const std::shared_ptr<DrawComponent>& comp) {
    std::shared_ptr<DrawImage> a = std::static_pointer_cast<DrawImage>(comp);
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
