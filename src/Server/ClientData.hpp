#pragma once
#include "../SharedTypes.hpp"
#include <Helpers/Serializers.hpp>
#include "../CoordSpaceHelper.hpp"
#include "../ResourceManager.hpp"
#include <Helpers/Networking/NetServer.hpp>
#include <chrono>

class MainServer;

class ClientData {
    public:
        CoordSpaceHelper camCoords;
        Vector2f windowSize;
        Vector2f cursorPos;

        Vector3f cursorColor;
        std::string displayName;
        ServerClientID id = {0, 0};

        std::queue<ResourceData> pendingResourceData;
        std::queue<ServerClientID> pendingResourceID;

        template <typename Archive> void serialize(Archive& a) {
            a(camCoords, windowSize, cursorPos, cursorColor, displayName);
        }

        ServerClientID get_next_id();
};

