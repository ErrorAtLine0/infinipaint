#pragma once
#include "Server/MainServer.hpp"
#include <Helpers/Networking/NetClient.hpp>

class ConnectionManager {
    public:
        void init_local_p2p(World& initWorld, const std::string& serverLocalID);
        void connect_p2p(const std::string& serverFullID);
        void update();
        void client_add_recv_callback(uint32_t commandID, const NetClientRecvCallback &callback);
        template <typename... Args> void client_send_items_to_server(const std::string& channel, Args&&... items) {
            if(client)
                client->send_items_to_server(channel, items...);
        }
        bool is_client_disconnected();
        bool is_host_disconnected();
    private:
        std::shared_ptr<NetClient> client;
        std::unique_ptr<MainServer> localServer;
        std::chrono::steady_clock::time_point lastKeepAliveTime;
};
