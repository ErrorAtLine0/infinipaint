#pragma once
#include <Helpers/Networking/NetServer.hpp>
#include <chrono>
#include "ClientData.hpp"
#include "Helpers/Networking/NetLibrary.hpp"
#include "ServerData.hpp"

class MainServer {
    public:
        MainServer(World& initWorld, const std::string& serverLocalID);
        void update();
        ~MainServer();
        std::shared_ptr<NetServer> netServer;
        std::unordered_map<ServerPortionID, ClientData> clients;
    private:
        World& world;
        void ensure_display_name_unique(std::string& displayName);
        ServerData data;
        std::chrono::steady_clock::time_point lastKeepAliveSent;
};
