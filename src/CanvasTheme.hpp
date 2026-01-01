#pragma once
#include "cereal/archives/portable_binary.hpp"
#include <include/core/SkColor.h>
#include <Helpers/ConvertVec.hpp>
#include <Helpers/NetworkingObjects/NetObjOwnerPtr.hpp>
#include <Helpers/Serializers.hpp>
#include <Helpers/VersionNumber.hpp>

class DrawingProgram;
class World;

class CanvasTheme {
    public:
        CanvasTheme(World& w);
        void server_init_no_file();
        SkColor4f get_back_color() const;
        const SkColor4f& get_tool_front_color() const;
        void set_back_color(const Vector3f& newBackColor);
        void register_class();
        void read_create_message(cereal::PortableBinaryInputArchive& a);
        void write_create_message(cereal::PortableBinaryOutputArchive& a) const;
        void save_file(cereal::PortableBinaryOutputArchive& a) const;
        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version);
    private:
        void set_tool_front_color(DrawingProgram& drawP);
        struct BackColor {
            Vector3f c;
            template <typename Archive> void serialize(Archive& a) {
                a(c);
            }
        };
        NetworkingObjects::NetObjOwnerPtr<BackColor> backColor;
        SkColor4f toolFrontColor = {1.0f, 1.0f, 1.0f, 1.0f};
        World& world;
};
