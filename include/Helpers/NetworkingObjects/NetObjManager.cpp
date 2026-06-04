/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "NetObjManager.hpp"
#include "NetObjTemporaryPtr.hpp"

namespace NetworkingObjects {
    NetObjManager::NetObjManager(bool initIsServer):
        isServer(initIsServer),
        nextTypeID(0)
    {}
    bool NetObjManager::is_server() const {
        return isServer;
    }
    void NetObjManager::read_update_message(cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& clientReceivedFrom) {
        NetObjID id;
        a(id);
        auto it = objectData.find(id);
        if(it == objectData.end())
            return;
        typeList.get_net_type_data(isServer, it->second.netTypeID).readUpdateFunc(NetObjTemporaryPtr<void>(this, id, it->second.p), a, clientReceivedFrom);
    }
    void NetObjManager::set_client(std::shared_ptr<NetClient> initClient, MessageCommandType initUpdateCommandID) {
        client = initClient;
        updateCommandID = initUpdateCommandID;
    }
    void NetObjManager::set_server(std::shared_ptr<NetServer> initServer, MessageCommandType initUpdateCommandID) {
        server = initServer;
        updateCommandID = initUpdateCommandID;
    }
    void NetObjManager::disconnect() {
        server = nullptr;
        client = nullptr;
    }
    bool NetObjManager::is_connected() const {
        return (server && !server->is_disconnected()) || (client && !client->is_disconnected());
    }
    void NetObjManager::set_netid_reassign_callback(const std::function<void(const NetworkingObjects::NetObjID& oldID, const NetworkingObjects::NetObjID& newID)>& newNetIDReassignCallback) {
        netIDReassignCallback = newNetIDReassignCallback;
    }
    void NetObjManager::set_netobj_destroy_callback(const std::function<void(const NetworkingObjects::NetObjID& netID)>& newDestroyCallback) {
        destroyCallback = newDestroyCallback;
    }

    void NetObjManager::send_update_data_by_type(const std::string& channel, const std::shared_ptr<std::stringstream>& ss, SendUpdateType updateType, const std::shared_ptr<NetServer::ClientData>& specificClient) {
        if(client && updateType != SendUpdateType::SEND_TO_ALL)
            throw std::runtime_error("[NetObjManager::send_update_data_by_type] Client can only send to all (to server)");
        switch(updateType) {
            case SendUpdateType::SEND_TO_ALL:
                if(client)
                    client->send_string_stream_to_server(channel, ss);
                else
                    server->send_string_stream_to_all_clients(channel, ss);
                break;
            case SendUpdateType::SEND_TO_ALL_EXCEPT:
                server->send_string_stream_to_all_clients_except(specificClient, channel, ss);
                break;
            case SendUpdateType::SEND_TO_SPECIFIC_CLIENT:
                server->send_string_stream_to_client(specificClient, channel, ss);
                break;
        }
    }
}
