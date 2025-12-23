#include "ConnectionManager.hpp"
#include "Helpers/Networking/NetLibrary.hpp"
#include "CommandList.hpp"
#include <chrono>
#include "World.hpp"
#include <Helpers/Logger.hpp>

void ConnectionManager::connect_p2p(World& initWorld, const std::string& serverFullID) {
    client = std::make_shared<NetClient>(serverFullID);
    lastKeepAliveTime = std::chrono::steady_clock::now();
    NetLibrary::register_client(client);
    initWorld.netObjMan.set_client(client, SERVER_UPDATE_NETWORK_OBJECT, SERVER_UPDATE_MANY_NETWORK_OBJECTS);
}

void ConnectionManager::update() {
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

bool ConnectionManager::client_exists() {
    return client != nullptr;
}

NetLibrary::DownloadProgress ConnectionManager::client_get_resource_retrieval_progress() {
    return client->get_progress_into_fragmented_message(RESOURCE_COMMAND_CHANNEL);
}
