#pragma once
#include <Helpers/Networking/NetClient.hpp>

class World;

class ConnectionManager {
    public:
        void connect_p2p(World& initWorld, const std::string& serverFullID);
        void update();
        void client_add_recv_callback(uint32_t commandID, const NetClientRecvCallback &callback);
        template <typename... Args> void client_send_items_to_server(const std::string& channel, Args&&... items) {
            if(client)
                client->send_items_to_server(channel, items...);
        }
        bool is_client_disconnected();
        bool host_exists();
        bool client_exists();
        NetLibrary::DownloadProgress client_get_resource_retrieval_progress();
    private:
        std::shared_ptr<NetClient> client;
        std::chrono::steady_clock::time_point lastKeepAliveTime;
};
