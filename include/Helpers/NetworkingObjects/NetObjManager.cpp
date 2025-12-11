#include "NetObjManager.hpp"
#include "NetObjTemporaryPtr.hpp"

namespace NetworkingObjects {
    NetObjManager::NetObjManager(std::shared_ptr<NetObjManagerTypeList> initTypeList, bool initIsServer):
        isServer(initIsServer),
        typeList(initTypeList),
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
        typeList->get_net_type_data(isServer, it->second.netTypeID).readUpdateFunc(NetObjTemporaryPtr<void>(this, id, it->second.p), a, clientReceivedFrom);
    }
    void NetObjManager::set_client(std::shared_ptr<NetClient> initClient, MessageCommandType initUpdateCommandID) {
        client = initClient;
        updateCommandID = initUpdateCommandID;
    }
    void NetObjManager::set_server(std::shared_ptr<NetServer> initServer, MessageCommandType initUpdateCommandID) {
        server = initServer;
        updateCommandID = initUpdateCommandID;
    }
}
