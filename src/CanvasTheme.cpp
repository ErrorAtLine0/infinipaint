#include "CanvasTheme.hpp"
#include "DrawingProgram/DrawingProgram.hpp"
#include <Helpers/HsvRgb.hpp>
#include "MainProgram.hpp"
#include "Helpers/Networking/NetLibrary.hpp"
#include "World.hpp"

using namespace NetworkingObjects;

CanvasTheme::CanvasTheme(World& w):
    world(w)
{}

void CanvasTheme::init() {
    if(world.netObjMan.is_server()) {
        backColor = world.netObjMan.make_obj_direct<BackColor>(world.main.defaultCanvasBackgroundColor);
        set_tool_front_color(world.drawProg);
    }
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
        world.delayedUpdateObjectManager.send_update_to_all<BackColor>(backColor, true);
        set_tool_front_color(world.drawProg);
    }
}

void CanvasTheme::set_tool_front_color(DrawingProgram& drawP) {
    Vector3f newHSV = rgb_to_hsv<Vector3f>(backColor->c);
    SkColor4f oldToolFrontColor = toolFrontColor;
    if(newHSV.z() >= 0.6f)
        toolFrontColor = SkColor4f{0.0f, 0.0f, 0.0f, 1.0f};
    else
        toolFrontColor = SkColor4f{1.0f, 1.0f, 1.0f, 1.0f};
    if(oldToolFrontColor != toolFrontColor)
        drawP.clear_draw_cache();
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
