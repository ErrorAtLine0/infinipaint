#include "NetObjManager.hpp"
#include "NetObjPtr.hpp"

namespace NetworkingObjects {
    NetObjManager::NetObjManager(std::shared_ptr<NetObjIDGenerator> initIDGen, std::shared_ptr<NetObjManagerTypeList> initTypeList, bool initIsServer):
        isServer(initIsServer),
        idGen(initIDGen),
        typeList(initTypeList),
        nextTypeID(0)
    {}
    void NetObjManager::read_update_message(cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& clientReceivedFrom) {
        NetObjID id;
        a(id);
        auto it = objectData.find(id);
        if(it == objectData.end())
            return;
        auto& netTypeIDData = typeList->netTypeIDData[it->second.netTypeID];
        if(isServer)
            netTypeIDData.readUpdateFuncServer(NetObjPtr<void>(this, id, it->second.p), a, clientReceivedFrom);
        else
            netTypeIDData.readUpdateFuncClient(NetObjPtr<void>(this, id, it->second.p), a, clientReceivedFrom);
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
