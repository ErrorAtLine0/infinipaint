#pragma once
#include <cstdint>
#include <cereal/archives/portable_binary.hpp>
#include <Helpers/VersionNumber.hpp>

class CanvasComponent {
    public:
        enum class CompType : uint8_t {
            BRUSHSTROKE = 0,
            RECTANGLE,
            ELLIPSE,
            TEXTBOX,
            IMAGE
        };
    protected:
        virtual void write_update() const;
        virtual void read_update();
        virtual void save_file(cereal::PortableBinaryOutputArchive& a) const;
        virtual void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version);
};
