#include "MainServer.hpp"
#include "CommandList.hpp"
#include "Helpers/HsvRgb.hpp"
#include "Helpers/Networking/NetLibrary.hpp"
#include "ServerData.hpp"
#include "../SharedTypes.hpp"
#include "../CoordSpaceHelper.hpp"
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/unordered_set.hpp>
#include <Helpers/Serializers.hpp>
#include <Helpers/Random.hpp>
#include <chrono>
#include <limits>
#include <fstream>
#include "../World.hpp"

MainServer::MainServer(World& initWorld, const std::string& serverLocalID):
    world(initWorld)
{
    data.canvasScale = world.canvasScale;

    lastKeepAliveSent = std::chrono::steady_clock::now();

    netServer = std::make_shared<NetServer>(serverLocalID);
    NetLibrary::register_server(netServer);
    
    world.netObjMan.set_server(netServer, CLIENT_UPDATE_NETWORK_OBJECT);

    netServer->add_recv_callback(SERVER_INITIAL_DATA, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        bool isDirectConnect;
        ClientData newClient;
        Vector3f hsv;
        hsv[0] = Random::get().real_range(0.0f, 360.0f);
        hsv[1] = Random::get().real_range(0.3f, 0.7f);
        hsv[2] = 1.0;
        newClient.cursorColor = hsv_to_rgb<Vector3f>(hsv);
        newClient.canvasScale = data.canvasScale;
        message(newClient.displayName, isDirectConnect);
        ensure_display_name_unique(newClient.displayName);

        if(isDirectConnect) {
            fileDisplayName = newClient.displayName;
            message(newClient.camCoords, newClient.windowSize);
        }
        else {
            auto& [randomClientServerID, randomClient] = *clients.begin();
            newClient.camCoords = randomClient.camCoords;
            newClient.windowSize = randomClient.windowSize;
        }

        newClient.serverID = Random::get().int_range<ServerPortionID>(1, std::numeric_limits<ServerPortionID>::max());
        client->customID = newClient.serverID;
        netServer->send_items_to_all_clients_except(client, RELIABLE_COMMAND_CHANNEL, CLIENT_USER_CONNECT, newClient.serverID, newClient.displayName, newClient.cursorColor);
        if(isDirectConnect)
            netServer->send_items_to_client(client, RELIABLE_COMMAND_CHANNEL, CLIENT_INITIAL_DATA, isDirectConnect, newClient.serverID, newClient.cursorColor);
        else {
            auto ss(std::make_shared<std::stringstream>());
            {
                cereal::PortableBinaryOutputArchive a(*ss);
                a(CLIENT_INITIAL_DATA, isDirectConnect, newClient.serverID, newClient.cursorColor, newClient.displayName, newClient.camCoords, newClient.windowSize, fileDisplayName, clients, data);
                world.bMan.bookmarks.write_create_message(a);
                world.gridMan.grids.write_create_message(a);
                world.drawProg.write_components_server(a);
                world.canvasTheme.write_create_message(a);
            }
            netServer->send_string_stream_to_client(client, RELIABLE_COMMAND_CHANNEL, ss);
            for(auto& r : world.rMan.resource_list()) {
                netServer->send_items_to_client(client, RESOURCE_COMMAND_CHANNEL, CLIENT_NEW_RESOURCE_ID, r.get_net_id());
                netServer->send_items_to_client(client, RESOURCE_COMMAND_CHANNEL, CLIENT_NEW_RESOURCE_DATA, *r);
            }
        }
        clients.emplace(newClient.serverID, newClient);
    });
    netServer->add_recv_callback(SERVER_UPDATE_NETWORK_OBJECT, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        world.netObjMan.read_update_message(message, client);
    });
    netServer->add_recv_callback(SERVER_MOVE_CAMERA, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        auto& c = clients[client->customID];
        message(c.camCoords, c.windowSize);
        netServer->send_items_to_all_clients_except(client, RELIABLE_COMMAND_CHANNEL, CLIENT_MOVE_CAMERA, c.serverID, c.camCoords, c.windowSize);
    });
    netServer->add_recv_callback(SERVER_MOVE_SCREEN_MOUSE, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        auto& c = clients[client->customID];
        message(c.cursorPos);
        netServer->send_items_to_all_clients_except(client, UNRELIABLE_COMMAND_CHANNEL, CLIENT_MOVE_SCREEN_MOUSE, c.serverID, c.cursorPos);
    });
    netServer->add_recv_callback(SERVER_KEEP_ALIVE, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
    });
    netServer->add_disconnect_callback([&](std::shared_ptr<NetServer::ClientData> client) {
        auto it = clients.find(client->customID);
        if(it != clients.end()) {
            netServer->send_items_to_all_clients_except(client, RELIABLE_COMMAND_CHANNEL, CLIENT_USER_DISCONNECT, client->customID);
            clients.erase(it);
        }
    });
}

void MainServer::ensure_display_name_unique(std::string& displayName) {
    for(;;) {
        bool isUnique = true;
        for(auto& [id, client] : clients) {
            if(client.displayName == displayName) {
                size_t leftParenthesisIndex = displayName.find_last_of('(');
                size_t rightParenthesisIndex = displayName.find_last_of(')');
                bool failToIncrement = true;
                if(leftParenthesisIndex != std::string::npos && rightParenthesisIndex != std::string::npos && leftParenthesisIndex < rightParenthesisIndex) {
                    std::string numStr = displayName.substr(leftParenthesisIndex + 1, rightParenthesisIndex - leftParenthesisIndex - 1);
                    bool isAllDigits = true;
                    for(char c : numStr) {
                        if(!isdigit(c)) {
                            isAllDigits = false;
                            break;
                        }
                    }
                    if(isAllDigits) {
                        try {
                            int s = std::stoi(numStr);
                            s++;
                            displayName = displayName.substr(0, leftParenthesisIndex);
                            displayName += "(" + std::to_string(s) + ")";
                            failToIncrement = false;
                        }
                        catch(...) {}
                    }
                }
                if(failToIncrement)
                    displayName += " (2)";
                isUnique = false;
                break;
            }
        }
        if(isUnique)
            break;
    }
}

void MainServer::update() {
    netServer->update();
    if(std::chrono::steady_clock::now() - lastKeepAliveSent > std::chrono::seconds(2)) {
        netServer->send_items_to_all_clients(UNRELIABLE_COMMAND_CHANNEL, CLIENT_KEEP_ALIVE);
        lastKeepAliveSent = std::chrono::steady_clock::now();
    }
}

MainServer::~MainServer() {
    //netServer->close_server();
}
