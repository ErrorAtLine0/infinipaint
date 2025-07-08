#include "MainServer.hpp"
#include "CommandList.hpp"
#include "Helpers/HsvRgb.hpp"
#include "Helpers/Networking/NetLibrary.hpp"
#include "ServerData.hpp"
#include "../DrawComponents/DrawBrushStroke.hpp"
#include "../DrawComponents/DrawRectangle.hpp"
#include "../DrawComponents/DrawEllipse.hpp"
#include "../SharedTypes.hpp"
#include "../CoordSpaceHelper.hpp"
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/string.hpp>
#include <Helpers/Serializers.hpp>
#include <Helpers/Random.hpp>
#include <chrono>
#include <limits>
#include <fstream>
#include "../DrawComponents/DrawComponent.hpp"
#include "../World.hpp"

WorldVec cursorPos;

void ServerData::save(cereal::PortableBinaryOutputArchive& a) const {
    a((uint64_t)components.size());
    for(uint64_t i = 0; i < components.size(); i++)
        a(components[i].second->get_type(), components[i].first, components[i].second->coords, *(components[i].second));
    a(bookmarks);
}

void ServerData::load(cereal::PortableBinaryInputArchive& a) {
    uint64_t compCount;
    a(compCount);
    for(uint64_t i = 0; i < compCount; i++) {
        DrawComponentType t;
        ServerClientID id;
        a(t, id);
        auto newComp = DrawComponent::allocate_comp_type(t);
        a(newComp->coords, *newComp);
        components.emplace_back(id, newComp);
    }
    a(bookmarks);
}

ClientPortionID ServerData::get_max_id(ServerPortionID serverID) const {
    ClientPortionID maxClientID = 0;
    for(auto& p : resources) {
        if(p.first.first == serverID)
            maxClientID = std::max(maxClientID, p.first.second);
    }
    for(auto& p : components) {
        if(p.first.first == serverID)
            maxClientID = std::max(maxClientID, p.first.second);
    }
    return maxClientID;
}

MainServer::MainServer(World& initWorld, const std::string& serverLocalID):
    world(initWorld)
{
    for(auto& c : world.drawProg.components.client_list())
        data.components.emplace_back(c->id, c->obj->copy());
    data.bookmarks = world.bMan.bookmark_list();
    data.resources = world.rMan.resource_list();

    lastKeepAliveSent = std::chrono::steady_clock::now();

    netServer = std::make_shared<NetServer>(serverLocalID);
    NetLibrary::register_server(netServer);

    netServer->add_recv_callback(SERVER_INITIAL_DATA, [&](NetServer::ClientData& client, cereal::PortableBinaryInputArchive& message) {
        bool isDirectConnect;
        ClientData newClient;
        Vector3f hsv;
        hsv[0] = Random::get().real_range(0.0f, 360.0f);
        hsv[1] = Random::get().real_range(0.3f, 0.7f);
        hsv[2] = 1.0;
        newClient.cursorColor = hsv_to_rgb<Vector3f>(hsv);
        message(newClient.displayName, isDirectConnect);
        ensure_display_name_unique(newClient.displayName);

        if(isDirectConnect)
            fileDisplayName = newClient.displayName;

        newClient.id.first = Random::get().int_range<ServerPortionID>(1, std::numeric_limits<ServerPortionID>::max());
        client.customID = newClient.id.first;
        newClient.id.second = data.get_max_id(newClient.id.first);
        netServer->send_items_to_all_clients_except(client, RELIABLE_COMMAND_CHANNEL, CLIENT_USER_CONNECT, newClient.id.first, newClient.displayName, newClient.cursorColor);
        if(isDirectConnect)
            netServer->send_items_to_client(client, RELIABLE_COMMAND_CHANNEL, CLIENT_INITIAL_DATA, isDirectConnect, newClient.id.first, newClient.cursorColor);
        else {
            netServer->send_items_to_client(client, RELIABLE_COMMAND_CHANNEL, CLIENT_INITIAL_DATA, isDirectConnect, newClient.id.first, newClient.cursorColor, newClient.displayName, fileDisplayName, clients, data);
            for(auto& [id, rData] : data.resources)
                netServer->send_items_to_client(client, RESOURCE_COMMAND_CHANNEL, CLIENT_NEW_RESOURCE, id, rData);
        }
        clients.emplace(newClient.id.first, newClient);
    });
    netServer->add_recv_callback(SERVER_MOVE_MOUSE, [&](NetServer::ClientData& client, cereal::PortableBinaryInputArchive& message) {
        auto& c = clients[client.customID];
        message(c.camCoords, c.windowSize, c.cursorPos);
        netServer->send_items_to_all_clients_except(client, UNRELIABLE_COMMAND_CHANNEL, CLIENT_MOVE_MOUSE, c.id.first, c.camCoords, c.windowSize, c.cursorPos);
    });
    netServer->add_recv_callback(SERVER_PLACE_COMPONENT, [&](NetServer::ClientData& client, cereal::PortableBinaryInputArchive& message) {
        uint64_t placement;
        DrawComponentType type;
        message(placement, type);
        auto newComp = DrawComponent::allocate_comp_type(type);
        message(newComp->coords, *newComp);
        ServerClientID id = clients[client.customID].get_next_id();
        placement = std::min<uint64_t>(placement, data.components.size());
        data.components.insert(data.components.begin() + placement, {id, newComp});
        newComp->server_send_place(*this, id, placement);
    });
    netServer->add_recv_callback(SERVER_ERASE_COMPONENT, [&](NetServer::ClientData& client, cereal::PortableBinaryInputArchive& message) {
        ServerClientID compToRemove;
        message(compToRemove);
        std::erase_if(data.components, [&](auto& c){ return c.first == compToRemove; });
        DrawComponent::server_send_erase(*this, compToRemove);
    });
    netServer->add_recv_callback(SERVER_TRANSFORM_COMPONENT, [&](NetServer::ClientData& client, cereal::PortableBinaryInputArchive& message) {
        bool isTemp;
        ServerClientID idToTransform;
        message(isTemp, idToTransform);
        auto a = std::find_if(data.components.begin(), data.components.end(), [&](auto& c) {
            return (c.first == idToTransform);
        });
        if(a != data.components.end()) {
            std::shared_ptr<DrawComponent>& comp = (*a).second;
            message(comp->coords);
            if(isTemp)
                comp->server_send_transform_temp(*this, idToTransform);
            else
                comp->server_send_transform_final(*this, idToTransform);
        }
    });
    netServer->add_recv_callback(SERVER_UPDATE_COMPONENT, [&](NetServer::ClientData& client, cereal::PortableBinaryInputArchive& message) {
        bool isTemp;
        ServerClientID idToTransform;
        message(isTemp, idToTransform);
        auto a = std::find_if(data.components.begin(), data.components.end(), [&](auto& c) {
            return (c.first == idToTransform);
        });
        if(a != data.components.end()) {
            std::shared_ptr<DrawComponent>& comp = (*a).second;
            message(*comp);
            if(isTemp)
                comp->server_send_update_temp(*this, idToTransform);
            else
                comp->server_send_update_final(*this, idToTransform);
        }
    });
    netServer->add_recv_callback(SERVER_RESOURCE_INIT, [&](NetServer::ClientData& client, cereal::PortableBinaryInputArchive& message) {
        auto& c = clients[client.customID];
        ServerClientID newID = c.get_next_id();
        if(c.pendingResourceData.empty())
            c.pendingResourceID.emplace(newID);
        else {
            netServer->send_items_to_all_clients_except(client, RESOURCE_COMMAND_CHANNEL, CLIENT_NEW_RESOURCE, newID, c.pendingResourceData.front());
            data.resources.emplace(newID, c.pendingResourceData.front());
            c.pendingResourceData.pop();
        }
    });
    netServer->add_recv_callback(SERVER_RESOURCE_DATA, [&](NetServer::ClientData& client, cereal::PortableBinaryInputArchive& message) {
        ResourceData resourceData;
        message(resourceData);
        auto& c = clients[client.customID];
        if(c.pendingResourceID.empty()) {
            c.pendingResourceData.emplace();
            c.pendingResourceData.back() = std::move(resourceData);
        }
        else {
            netServer->send_items_to_all_clients_except(client, RESOURCE_COMMAND_CHANNEL, CLIENT_NEW_RESOURCE, c.pendingResourceID.front(), resourceData);
            data.resources.emplace(c.pendingResourceID.front(), resourceData);
            c.pendingResourceID.pop();
        }
    });
    netServer->add_recv_callback(SERVER_CHAT_MESSAGE, [&](NetServer::ClientData& client, cereal::PortableBinaryInputArchive& message) {
        std::string chatMessage;
        Vector4f c{1.0f, 1.0f, 1.0f, 1.0f};
        message(chatMessage);
        netServer->send_items_to_all_clients_except(client, RELIABLE_COMMAND_CHANNEL, CLIENT_CHAT_MESSAGE, chatMessage, c);
    });
    netServer->add_recv_callback(SERVER_NEW_BOOKMARK, [&](NetServer::ClientData& client, cereal::PortableBinaryInputArchive& message) {
        std::string name;
        message(name);
        Bookmark& b = data.bookmarks[name];
        message(b);
        netServer->send_items_to_all_clients(RELIABLE_COMMAND_CHANNEL, CLIENT_NEW_BOOKMARK, name, b);
    });
    netServer->add_recv_callback(SERVER_REMOVE_BOOKMARK, [&](NetServer::ClientData& client, cereal::PortableBinaryInputArchive& message) {
        std::string name;
        message(name);
        data.bookmarks.erase(name);
        netServer->send_items_to_all_clients(RELIABLE_COMMAND_CHANNEL, CLIENT_REMOVE_BOOKMARK, name);
    });
    netServer->add_recv_callback(SERVER_KEEP_ALIVE, [&](NetServer::ClientData& client, cereal::PortableBinaryInputArchive& message) {
    });
    netServer->add_disconnect_callback([&](NetServer::ClientData& client) {
        auto it = clients.find(client.customID);
        if(it != clients.end()) {
            netServer->send_items_to_all_clients_except(client, RELIABLE_COMMAND_CHANNEL, CLIENT_USER_DISCONNECT, client.customID);
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
    for(auto& c : data.components)
        c.second->server_update(*this, c.first);
    if(std::chrono::steady_clock::now() - lastKeepAliveSent > std::chrono::seconds(2)) {
        netServer->send_items_to_all_clients(UNRELIABLE_COMMAND_CHANNEL, CLIENT_KEEP_ALIVE);
        lastKeepAliveSent = std::chrono::steady_clock::now();
    }
}

MainServer::~MainServer() {
    //netServer->close_server();
}
