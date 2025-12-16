#pragma once
#include "../SharedTypes.hpp"
#include <Helpers/Serializers.hpp>
#include "../CoordSpaceHelper.hpp"
#include "../ResourceManager.hpp"
#include <Helpers/Networking/NetServer.hpp>
#include <chrono>

class ClientData {
    public:
        CoordSpaceHelper camCoords;
        Vector2f windowSize = {1000.0f, 1000.0f};
        Vector2f cursorPos = {1.0f, 1.0f};

        Vector3f cursorColor = {1.0f, 1.0f, 1.0f};
        std::string displayName;
        ServerPortionID serverID = 0;

        uint64_t canvasScale = 0;

        template <typename Archive> void serialize(Archive& a) {
            a(camCoords, windowSize, cursorPos, cursorColor, displayName);
        }
};

