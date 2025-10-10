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
#include <cereal/types/unordered_set.hpp>
#include <Helpers/Serializers.hpp>
#include <Helpers/Random.hpp>
#include <chrono>
#include <limits>
#include <fstream>
#include "../DrawComponents/DrawComponent.hpp"
#include "../World.hpp"

MainServer::MainServer(World& initWorld, const std::string& serverLocalID):
    world(initWorld)
{
    for(auto& c : world.drawProg.components.client_list()) {
        data.components.emplace_back(c->obj->copy());
        data.components.back()->id = c->obj->id;
        data.idToComponentMap.emplace(data.components.back()->id, data.components.back());
    }
    data.canvasScale = world.canvasScale;
    data.bookmarks = world.bMan.bookmark_list();
    data.grids = world.gridMan.grids;
    data.resources = world.rMan.resource_list();
    data.canvasBackColor = convert_vec3<Vector3f>(world.canvasTheme.backColor);

    lastKeepAliveSent = std::chrono::steady_clock::now();

    netServer = std::make_shared<NetServer>(serverLocalID);
    NetLibrary::register_server(netServer);

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

        if(isDirectConnect)
            fileDisplayName = newClient.displayName;

        newClient.serverID = Random::get().int_range<ServerPortionID>(1, std::numeric_limits<ServerPortionID>::max());
        client->customID = newClient.serverID;
        netServer->send_items_to_all_clients_except(client, RELIABLE_COMMAND_CHANNEL, CLIENT_USER_CONNECT, newClient.serverID, newClient.displayName, newClient.cursorColor);
        if(isDirectConnect)
            netServer->send_items_to_client(client, RELIABLE_COMMAND_CHANNEL, CLIENT_INITIAL_DATA, isDirectConnect, newClient.serverID, newClient.cursorColor);
        else {
            netServer->send_items_to_client(client, RELIABLE_COMMAND_CHANNEL, CLIENT_INITIAL_DATA, isDirectConnect, newClient.serverID, newClient.cursorColor, newClient.displayName, fileDisplayName, clients, data);
            for(auto& [id, rData] : data.resources) {
                netServer->send_items_to_client(client, RESOURCE_COMMAND_CHANNEL, CLIENT_NEW_RESOURCE_ID, id);
                netServer->send_items_to_client(client, RESOURCE_COMMAND_CHANNEL, CLIENT_NEW_RESOURCE_DATA, rData);
            }
        }
        clients.emplace(newClient.serverID, newClient);
    });
    netServer->add_recv_callback(SERVER_MOVE_MOUSE, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        auto& c = clients[client->customID];
        message(c.camCoords, c.windowSize, c.cursorPos);
        netServer->send_items_to_all_clients_except(client, UNRELIABLE_COMMAND_CHANNEL, CLIENT_MOVE_MOUSE, c.serverID, c.camCoords, c.windowSize, c.cursorPos);
    });
    netServer->add_recv_callback(SERVER_PLACE_SINGLE_COMPONENT, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        uint64_t placement;
        DrawComponentType type;
        message(placement, type);
        auto newComp = DrawComponent::allocate_comp_type(type);
        message(newComp->id, newComp->coords, *newComp);
        placement = std::min<uint64_t>(placement, data.components.size());
        data.components.insert(data.components.begin() + placement, newComp);
        data.idToComponentMap.emplace(newComp->id, newComp);

        auto& c = clients[client->customID];
        if(c.canvasScale < data.canvasScale)
            newComp->scale_up(FixedPoint::pow_int(CANVAS_SCALE_UP_STEP, data.canvasScale - c.canvasScale));

        newComp->server_send_place(*this, placement);
    });
    netServer->add_recv_callback(SERVER_PLACE_MANY_COMPONENTS, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        std::vector<CollabListType::ObjectInfoPtr> objOrderedVector;
        SendOrderedComponentVectorOp a{&objOrderedVector};
        message(a);

        std::vector<std::shared_ptr<DrawComponent>> objOrderedComponentPairs;
        for(auto& o : objOrderedVector)
            objOrderedComponentPairs.emplace_back(o->obj);

        uint64_t startPoint = 0;

        if(objOrderedVector.empty())
            return;

        data.idToComponentMap.emplace(objOrderedVector.front()->obj->id, objOrderedVector.front()->obj);

        for(uint64_t i = 1; i < static_cast<uint64_t>(objOrderedVector.size()); i++) {
            auto& obj = objOrderedVector[i];
            data.idToComponentMap.emplace(obj->obj->id, obj->obj);
            if(obj->pos - objOrderedVector[startPoint]->pos != (i - startPoint)) {
                uint64_t insertPosition = std::min<uint64_t>(data.components.size(), objOrderedVector[startPoint]->pos);
                data.components.insert(data.components.begin() + insertPosition, objOrderedComponentPairs.begin() + startPoint, objOrderedComponentPairs.begin() + i);
                startPoint = i;
            }
        }

        data.components.insert(data.components.begin() + std::min<uint64_t>(data.components.size(), objOrderedVector[startPoint]->pos), objOrderedComponentPairs.begin() + startPoint, objOrderedComponentPairs.end());

        startPoint = 0;

        for(uint64_t i = 0; i < data.components.size(); i++) {
            if(startPoint >= objOrderedVector.size())
                break;
            auto& c = data.components[i];
            auto& o = objOrderedVector[startPoint];
            if(c == o->obj) {
                o->pos = i;
                startPoint++;
            }
        }

        auto& c = clients[client->customID];
        if(c.canvasScale < data.canvasScale) {
            for(auto& o : objOrderedVector)
                o->obj->scale_up(FixedPoint::pow_int(CANVAS_SCALE_UP_STEP, data.canvasScale - c.canvasScale));
        }

        DrawComponent::server_send_place_many(*this, objOrderedVector);
    });
    netServer->add_recv_callback(SERVER_ERASE_SINGLE_COMPONENT, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        ServerClientID compToRemove;
        message(compToRemove);
        DrawComponent::server_send_erase(*this, compToRemove);
        data.idToComponentMap.erase(compToRemove);
        std::erase_if(data.components, [&](auto& p) {
            return p->id == compToRemove;
        });
    });
    netServer->add_recv_callback(SERVER_ERASE_MANY_COMPONENTS, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        std::unordered_set<ServerClientID> idsToRemove;
        message(idsToRemove);
        DrawComponent::server_send_erase_set(*this, idsToRemove);
        for(auto& id : idsToRemove)
            data.idToComponentMap.erase(id);
        std::erase_if(data.components, [&](auto& p) {
            return idsToRemove.contains(p->id);
        });
    });
    netServer->add_recv_callback(SERVER_TRANSFORM_MANY_COMPONENTS, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        std::vector<std::pair<ServerClientID, CoordSpaceHelper>> transformsToSend;
        message(transformsToSend);
        for(auto& t : transformsToSend) {
            auto it = data.idToComponentMap.find(t.first);
            if(it != data.idToComponentMap.end())
                it->second->coords = t.second;
        }
        auto& c = clients[client->customID];
        if(c.canvasScale < data.canvasScale) {
            for(auto& [id, coord] : transformsToSend)
                coord.scale_about(WorldVec{0, 0}, FixedPoint::pow_int(CANVAS_SCALE_UP_STEP, data.canvasScale - c.canvasScale), true);
        }
        DrawComponent::server_send_transform_many(*this, transformsToSend);
    });
    netServer->add_recv_callback(SERVER_UPDATE_COMPONENT, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        bool isFinalUpdate;
        ServerClientID idToUpdate;
        message(isFinalUpdate, idToUpdate);
        auto it = data.idToComponentMap.find(idToUpdate);
        if(it != data.idToComponentMap.end()) {
            std::shared_ptr<DrawComponent>& comp = it->second;
            message(*comp);
            comp->server_send_update(*this, isFinalUpdate);
        }
    });
    netServer->add_recv_callback(SERVER_NEW_RESOURCE_ID, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        auto& c = clients[client->customID];
        message(c.pendingResourceID);
        if(c.pendingResourceID == ServerClientID{0, 0})
            throw std::runtime_error("[MainServer::MainServer] Received a null resource ID");
    });
    netServer->add_recv_callback(SERVER_NEW_RESOURCE_DATA, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        ResourceData resourceData;
        message(resourceData);
        auto& c = clients[client->customID];
        if(c.pendingResourceID == ServerClientID{0, 0})
            throw std::runtime_error("[MainServer::MainServer] Received data for a null resource ID");
        netServer->send_items_to_all_clients_except(client, RESOURCE_COMMAND_CHANNEL, CLIENT_NEW_RESOURCE_ID, c.pendingResourceID);
        netServer->send_items_to_all_clients_except(client, RESOURCE_COMMAND_CHANNEL, CLIENT_NEW_RESOURCE_DATA, resourceData);
        data.resources.emplace(c.pendingResourceID, resourceData);
        c.pendingResourceID = ServerClientID{0, 0};
    });
    netServer->add_recv_callback(SERVER_CHAT_MESSAGE, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        std::string chatMessage;
        Vector4f c{1.0f, 1.0f, 1.0f, 1.0f};
        message(chatMessage);
        netServer->send_items_to_all_clients_except(client, RELIABLE_COMMAND_CHANNEL, CLIENT_CHAT_MESSAGE, chatMessage, c);
    });
    netServer->add_recv_callback(SERVER_NEW_BOOKMARK, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        std::string name;
        message(name);
        Bookmark& b = data.bookmarks[name];
        message(b);

        auto& c = clients[client->customID];
        if(c.canvasScale < data.canvasScale)
            b.scale_up(FixedPoint::pow_int(CANVAS_SCALE_UP_STEP, data.canvasScale - c.canvasScale));

        netServer->send_items_to_all_clients(RELIABLE_COMMAND_CHANNEL, CLIENT_NEW_BOOKMARK, name, b);
    });
    netServer->add_recv_callback(SERVER_REMOVE_BOOKMARK, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        std::string name;
        message(name);
        data.bookmarks.erase(name);
        netServer->send_items_to_all_clients(RELIABLE_COMMAND_CHANNEL, CLIENT_REMOVE_BOOKMARK, name);
    });
    netServer->add_recv_callback(SERVER_SET_GRID, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        ServerClientID gID;
        message(gID);
        WorldGrid& g = data.grids[gID];
        message(g);

        auto& c = clients[client->customID];
        if(c.canvasScale < data.canvasScale)
            g.scale_up(FixedPoint::pow_int(CANVAS_SCALE_UP_STEP, data.canvasScale - c.canvasScale));

        netServer->send_items_to_all_clients(RELIABLE_COMMAND_CHANNEL, CLIENT_SET_GRID, gID, g);
    });
    netServer->add_recv_callback(SERVER_REMOVE_GRID, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        ServerClientID gID;
        message(gID);
        data.grids.erase(gID);
        netServer->send_items_to_all_clients(RELIABLE_COMMAND_CHANNEL, CLIENT_REMOVE_GRID, gID);
    });
    netServer->add_recv_callback(SERVER_CANVAS_COLOR, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        message(data.canvasBackColor);
        netServer->send_items_to_all_clients(RELIABLE_COMMAND_CHANNEL, CLIENT_CANVAS_COLOR, data.canvasBackColor);
    });
    netServer->add_recv_callback(SERVER_CANVAS_SCALE, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        auto& c = clients[client->customID];
        message(c.canvasScale);
        if(c.canvasScale > data.canvasScale) {
            data.scale_up(FixedPoint::pow_int(CANVAS_SCALE_UP_STEP, c.canvasScale - data.canvasScale));
            data.canvasScale = c.canvasScale;
            netServer->send_items_to_all_clients(RELIABLE_COMMAND_CHANNEL, CLIENT_CANVAS_SCALE, data.canvasScale);
        }
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

std::unordered_map<ServerClientID, NetLibrary::DownloadProgress> MainServer::resource_download_progress() {
    std::unordered_map<ServerClientID, NetLibrary::DownloadProgress> toRet;

    if(!netServer)
        return toRet;

    for(auto& [serverID, c]  : clients) {
        if(c.pendingResourceID != ServerClientID{0, 0}) {
            const std::vector<std::shared_ptr<NetServer::ClientData>>& clientListInNetServer = netServer->get_client_list();
            auto netClientIt = std::find_if(clientListInNetServer.begin(), clientListInNetServer.end(), [&serverID](auto& nC) {
                return nC->customID == serverID;
            });
            if(netClientIt != clientListInNetServer.end()) {
                auto& netClient = *netClientIt;
                toRet.emplace(c.pendingResourceID, netClient->get_progress_into_fragmented_message(RESOURCE_COMMAND_CHANNEL));
            }
        }
    }

    return toRet;
}

MainServer::~MainServer() {
    //netServer->close_server();
}
