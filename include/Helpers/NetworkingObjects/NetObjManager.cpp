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
    void NetObjManager::read_many_update_message(cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& clientReceivedFrom) {
        for(;;) {
            bool objExists;
            a(objExists);
            if(!objExists)
                break;
            read_update_message(a, clientReceivedFrom);
        }
    }
    void NetObjManager::set_client(std::shared_ptr<NetClient> initClient, MessageCommandType initUpdateCommandID, MessageCommandType initMultiUpdateCommandID) {
        client = initClient;
        updateCommandID = initUpdateCommandID;
        multiUpdateCommandID = initMultiUpdateCommandID;
    }
    void NetObjManager::set_server(std::shared_ptr<NetServer> initServer, MessageCommandType initUpdateCommandID, MessageCommandType initMultiUpdateCommandID) {
        server = initServer;
        updateCommandID = initUpdateCommandID;
        multiUpdateCommandID = initMultiUpdateCommandID;
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
    void NetObjManager::send_multi_update_messsage(std::function<void()> captureSendBlock, SendUpdateType updateType, const std::shared_ptr<NetServer::ClientData>& specificClient) {
        if(is_connected()) {
            std::shared_ptr<MultiUpdateData> d = std::make_shared<MultiUpdateData>();
            d->ss = std::make_shared<std::stringstream>(std::ios::binary | std::ios::out);
            d->outArchive = std::make_unique<cereal::PortableBinaryOutputArchive>(*d->ss);
            (*d->outArchive)(multiUpdateCommandID);
            multiUpdateData = d; // Using weak pointer here ensures that object will be freed
            captureSendBlock();
            if(!d->channelToSendTo.empty()) {
                (*d->outArchive)(false);
                d->outArchive = nullptr; // Destroy output archive to finalize serialization
                send_update_data_by_type(d->channelToSendTo, d->ss, updateType, specificClient);
            }
        }
        else
            captureSendBlock();
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
