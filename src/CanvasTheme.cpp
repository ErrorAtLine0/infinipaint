#include "CanvasTheme.hpp"
#include "DrawingProgram/DrawingProgram.hpp"
#include <Helpers/HsvRgb.hpp>
#include "MainProgram.hpp"
#include "Helpers/Networking/NetLibrary.hpp"
#include "World.hpp"

using namespace NetworkingObjects;

const char* visibleBlendModeCode = R"V(
	vec4 main(vec4 src, vec4 dst) {
        float value = max(max(dst.x, dst.y), dst.z) / dst.a;
        vec4 actualSrc;
        if(value > 0.6)
            actualSrc = vec4(vec3(0.0), src.a);
        else
            actualSrc = vec4(src.a);
        return actualSrc + (1.0 - actualSrc.a) * dst;
    }
)V";

sk_sp<SkBlender> CanvasTheme::get_visible_blend_mode() {
    static sk_sp<SkBlender> blender = nullptr;
    if(!blender) {
        auto effect = SkRuntimeEffect::MakeForBlender(SkString(visibleBlendModeCode));
  	    if(!effect.effect) {
            std::cout << "[CanvasTheme::get_visible_blend_mode] " << " blender construction error\n";
            std::cout << effect.errorText.c_str() << std::endl;
            throw std::runtime_error("Shader Compile Failure");
        }
        blender = effect.effect->makeBlender(nullptr);
    }
    return blender;
}


CanvasTheme::CanvasTheme(World& w):
    world(w)
{}

void CanvasTheme::server_init_no_file() {
    backColor = world.netObjMan.make_obj_direct<BackColor>(world.main.defaultCanvasBackgroundColor);
    set_tool_front_color(world.drawProg);
}

SkColor4f CanvasTheme::get_back_color() const {
    if(!backColor)
        return SkColor4f{world.main.defaultCanvasBackgroundColor.x(), world.main.defaultCanvasBackgroundColor.y(), world.main.defaultCanvasBackgroundColor.z(), 1.0f};
    else
        return SkColor4f{backColor->c.x(), backColor->c.y(), backColor->c.z(), 1.0f};
}

const SkColor4f& CanvasTheme::get_tool_front_color() const {
    return toolFrontColor;
}

void CanvasTheme::set_back_color(const Vector3f& newBackColor) {
    if(backColor && newBackColor != backColor->c) {
        backColor->c = newBackColor;
        world.delayedUpdateObjectManager.send_update_to_all<BackColor>(backColor, false);
        set_tool_front_color(world.drawProg);
    }
}

void CanvasTheme::set_tool_front_color(DrawingProgram& drawP) {
    Vector3f newHSV = rgb_to_hsv<Vector3f>(backColor->c);
    if(newHSV.z() >= 0.6f)
        toolFrontColor = SkColor4f{0.0f, 0.0f, 0.0f, 1.0f};
    else
        toolFrontColor = SkColor4f{1.0f, 1.0f, 1.0f, 1.0f};
    drawP.drawCache.clear_own_cached_surfaces();
}

void CanvasTheme::register_class() {
    world.delayedUpdateObjectManager.register_class<BackColor>(world.netObjMan, NetworkingObjects::DelayUpdateSerializedClassManager::CustomConstructors<BackColor>{
        .postUpdateFunc = [&](BackColor& o) {
            set_tool_front_color(world.drawProg);
        }
    });
}

void CanvasTheme::read_create_message(cereal::PortableBinaryInputArchive& a) {
    backColor = world.netObjMan.read_create_message<BackColor>(a, nullptr);
}

void CanvasTheme::write_create_message(cereal::PortableBinaryOutputArchive& a) const {
    backColor.write_create_message(a);
}

void CanvasTheme::save_file(cereal::PortableBinaryOutputArchive& a) const {
    a(convert_vec3<Vector3f>(get_back_color()));
}

void CanvasTheme::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    backColor = world.netObjMan.make_obj_direct<BackColor>(world.main.defaultCanvasBackgroundColor);
    set_tool_front_color(world.drawProg);

    if(version < VersionNumber(0, 1, 0))
        set_back_color(DEFAULT_CANVAS_BACKGROUND_COLOR);
    else {
        Vector3f canvasBackColor;
        a(canvasBackColor);
        const Vector3f OLD_DEFAULT_CANVAS_BACKGROUND_COLOR{0.12f, 0.12f, 0.12f};
        if(version < VersionNumber(0, 3, 0) && canvasBackColor == OLD_DEFAULT_CANVAS_BACKGROUND_COLOR)
            set_back_color(DEFAULT_CANVAS_BACKGROUND_COLOR);
        else
            set_back_color(canvasBackColor);
    }
}
