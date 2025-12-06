#include "NetObjManager.hpp"

namespace NetworkingObjects {
    NetObjManager::NetObjManager(std::shared_ptr<NetObjIDGenerator> initIDGen, std::shared_ptr<NetObjManagerTypeList> initTypeList):
        idGen(initIDGen),
        typeList(initTypeList),
        nextTypeID(0)
    {}
    void NetObjManager::read_update_message(cereal::PortableBinaryInputArchive& a) {
        NetObjID id;
        a(id);
        auto it = objectData.find(id);
        if(it == objectData.end())
            return;
        auto& netTypeIDData = typeList->netTypeIDData[it->second.netTypeID];
        netTypeIDData.readUpdateFunc(it->second.p.get(), a);
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
