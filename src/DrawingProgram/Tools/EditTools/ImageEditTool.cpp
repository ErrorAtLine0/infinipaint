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

#include "../../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../../GUIStuff/ElementHelpers/ButtonHelpers.hpp"

#ifdef __EMSCRIPTEN__
    #include <EmscriptenHelpers/emscripten_browser_file.h>
#endif

ImageEditTool::ImageEditTool(DrawingProgram& initDrawP, CanvasComponentContainer::ObjInfo* initComp):
    DrawingProgramEditToolBase(initDrawP, initComp)
{}

void ImageEditTool::edit_start(EditTool& editTool, std::any& prevData) {
    auto& a = static_cast<ImageCanvasComponent&>(comp->obj->get_comp());
    static Vector2f staticZero = {0.0f, 0.0f};
    static Vector2f staticOne = {1.0f, 1.0f};

    Affine2f transformMat = Translation2f(a.d.p1) * AlignedScaling2f(a.d.p2 - a.d.p1) * Affine2f::Identity();

    prevData = a.d;
    constexpr float MINIMUM_DISTANCE_BETWEEN_IMAGE_CROP_POINTS = 0.02f;
    editTool.add_point_handle({&a.d.cropP1, &staticZero, &a.d.cropP2, 0.0f, MINIMUM_DISTANCE_BETWEEN_IMAGE_CROP_POINTS, transformMat});
    editTool.add_point_handle({&a.d.cropP2, &a.d.cropP1, &staticOne, MINIMUM_DISTANCE_BETWEEN_IMAGE_CROP_POINTS, 0.0f, transformMat});
    a.d.editing = true;
    comp->obj->commit_update(drawP);
}

void ImageEditTool::commit_edit_updates(std::any& prevData) {
    auto& a = static_cast<ImageCanvasComponent&>(comp->obj->get_comp());
    a.d.editing = false;
    comp->obj->commit_update(drawP);
}

bool ImageEditTool::edit_update() {
    return true;
}

void ImageEditTool::edit_gui(Toolbar& t) {
    using namespace GUIStuff;
    using namespace ElementHelpers;

    ImageCanvasComponent& a = static_cast<ImageCanvasComponent&>(comp->obj->get_comp());
    auto& gui = drawP.world.main.g.gui;
    gui.new_id("edit tool image", [&] {
        text_label_centered(gui, "File Properties");
        auto resourceData = drawP.world.netObjMan.get_obj_temporary_ref_from_id<ResourceData>(a.d.imageID);

        if(resourceData) {
            text_label(gui, "Name: " + resourceData->name);
            text_button(gui, "file download", "Download file", {
                .wide = true,
                .onClick = [&] {
                    #ifdef __EMSCRIPTEN__
                        emscripten_browser_file::download(
                            resourceData->name,
                            "application/octet-binary",
                            *resourceData->data
                        );
                    #endif
                    t.open_file_selector("Download File", {{"Any File", "*"}}, [resourceData](const std::filesystem::path& p, const auto& e) {
                        SDL_SaveFile(p.string().c_str(), resourceData->data->c_str(), resourceData->data->size());
                    }, resourceData->name, true);
                }
            });
        }
        else
            text_label_centered(gui, "Loading resource...");
    });
}
