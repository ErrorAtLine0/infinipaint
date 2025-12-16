#pragma once
#include "cereal/archives/portable_binary.hpp"
#include "../SharedTypes.hpp"
#include "../ResourceManager.hpp"
#include "../WorldGrid.hpp"

class DrawComponent;

class ServerData {
    public:
        uint64_t canvasScale = 0;

        void save(cereal::PortableBinaryOutputArchive& a) const;
        void load(cereal::PortableBinaryInputArchive& a);
        void scale_up(const WorldScalar& scaleUpAmount);
};
