#pragma once
#include "cereal/archives/portable_binary.hpp"
#include "../SharedTypes.hpp"
#include "../ResourceManager.hpp"
#include "../WorldGrid.hpp"

class DrawComponent;

class ServerData {
    public:
        std::unordered_map<ServerClientID, ResourceData> resources;
        uint64_t canvasScale = 0;
        Vector3f canvasBackColor;

        void save(cereal::PortableBinaryOutputArchive& a) const;
        void load(cereal::PortableBinaryInputArchive& a);
        void scale_up(const WorldScalar& scaleUpAmount);
};
