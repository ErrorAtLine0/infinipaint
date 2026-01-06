#include "ImageEditTool.hpp"
#include "../../DrawingProgram.hpp"
#include "../../../CanvasComponents/ImageCanvasComponent.hpp"
#include "../../../World.hpp"
#include "../../../Toolbar.hpp"
#include "../../../MainProgram.hpp"
#include "../../../ResourceManager.hpp"
#include "../EditTool.hpp"
#include "DrawingProgramEditToolBase.hpp"
#include "Eigen/Geometry"
#include <fstream>

#ifdef __EMSCRIPTEN__
    #include <EmscriptenHelpers/emscripten_browser_file.h>
#endif

ImageEditTool::ImageEditTool(DrawingProgram& initDrawP):
    DrawingProgramEditToolBase(initDrawP)
{}

void ImageEditTool::edit_start(EditTool& editTool, CanvasComponentContainer::ObjInfo* comp, std::any& prevData) {
    auto& a = static_cast<ImageCanvasComponent&>(comp->obj->get_comp());
    static Vector2f staticZero = {0.0f, 0.0f};
    static Vector2f staticOne = {1.0f, 1.0f};

    Affine2f transformMat = Translation2f(a.d.p1) * AlignedScaling2f(a.d.p2 - a.d.p1) * Affine2f::Identity();

    prevData = a.d;
    editTool.add_point_handle({&a.d.cropP1, &staticZero, &a.d.cropP2, 0.0f, transformMat});
    editTool.add_point_handle({&a.d.cropP2, &a.d.cropP1, &staticOne, 0.0f, transformMat});
    a.d.editing = true;
    comp->obj->commit_update(drawP);
}

void ImageEditTool::commit_edit_updates(CanvasComponentContainer::ObjInfo* comp, std::any& prevData) {
    auto& a = static_cast<ImageCanvasComponent&>(comp->obj->get_comp());
    a.d.editing = false;
    comp->obj->commit_update(drawP);
}

bool ImageEditTool::edit_update(CanvasComponentContainer::ObjInfo* comp) {
    return true;
}

bool ImageEditTool::edit_gui(CanvasComponentContainer::ObjInfo* comp) {
    ImageCanvasComponent& a = static_cast<ImageCanvasComponent&>(comp->obj->get_comp());
    Toolbar& t = drawP.world.main.toolbar;
    t.gui.push_id("edit tool image");
    t.gui.text_label_centered("File Properties");
    auto resourceData = drawP.world.netObjMan.get_obj_temporary_ref_from_id<ResourceData>(a.d.imageID);

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
