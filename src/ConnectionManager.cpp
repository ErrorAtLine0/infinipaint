#include "ConnectionManager.hpp"
#include "Server/CommandList.hpp"
#include <chrono>

void ConnectionManager::init_local_p2p(World& initWorld, const std::string& serverLocalID) {
    localServer = std::make_unique<MainServer>(initWorld, serverLocalID);
    client = std::make_shared<NetClient>(*localServer->netServer);
    lastKeepAliveTime = std::chrono::steady_clock::now();
}

void ConnectionManager::connect_p2p(const std::string& serverFullID) {
    client = std::make_shared<NetClient>(serverFullID);
    lastKeepAliveTime = std::chrono::steady_clock::now();
    NetLibrary::register_client(client);
}

void ConnectionManager::update() {
    if(localServer)
        localServer->update();
    if(client) {
        client->update();
        if(std::chrono::steady_clock::now() - lastKeepAliveTime > std::chrono::seconds(2)) {
            client->send_items_to_server(UNRELIABLE_COMMAND_CHANNEL, SERVER_KEEP_ALIVE);
            lastKeepAliveTime = std::chrono::steady_clock::now();
        }
    }
}

void ConnectionManager::client_add_recv_callback(uint32_t commandID, const NetClientRecvCallback &callback) {
    if(client)
        client->add_recv_callback(commandID, callback);
}

bool ConnectionManager::is_client_disconnected() {
    return client && client->is_disconnected();
}

bool ConnectionManager::is_host_disconnected() {
    return localServer && localServer->netServer->is_disconnected();
}
