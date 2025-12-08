#pragma once
#include <compare>
#include <memory>
#include <unordered_map>
#include <functional>
#include <cereal/archives/portable_binary.hpp>
#include <chrono>
#include <vector>
#include <Helpers/Random.hpp>
#include "../Networking/NetClient.hpp"
#include "../Networking/NetServer.hpp"
#include "../Networking/NetLibrary.hpp"
#include "Helpers/NetworkingObjects/NetObjID.hpp"
#include "NetObjManagerTypeList.hpp"
#include "NetObjPtr.decl.hpp"
#include "../Networking/NetServer.hpp"

namespace NetworkingObjects {
    class NetObjManager {
        public:
            NetObjManager(std::shared_ptr<NetObjManagerTypeList> initTypeList, bool initIsServer);
            void read_update_message(cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& clientReceivedFrom);
            void set_client(std::shared_ptr<NetClient> initClient, MessageCommandType initUpdateCommandID);
            void set_server(std::shared_ptr<NetServer> initServer, MessageCommandType initUpdateCommandID);
            bool is_server() const;

            template <typename T> NetObjPtr<T> read_create_message(cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& clientReceivedFrom) {
                NetObjID id;
                NetTypeIDType typeID;
                a(id, typeID);
                auto& netTypeIDData = typeList->netTypeIDData[typeID];
                if(isServer)
                    return netTypeIDData.readConstructorFuncServer(*this, a, clientReceivedFrom).cast<T>();
                else
                    return netTypeIDData.readConstructorFuncClient(*this, a, clientReceivedFrom).cast<T>();
            }
            template <typename T> NetObjPtr<T> read_get_obj_from_message(cereal::PortableBinaryInputArchive& a) {
                NetObjID id;
                a(id);
                auto it = objectData.find(id);
                if(it == objectData.end())
                    return NetObjPtr<T>();
                return NetObjPtr<T>(this, id, std::static_pointer_cast<T>(it->second.p));
            }
            template <typename T, typename... Args> NetObjPtr<T> make_obj(Args&&... items) {
                auto sharedPtr = std::make_shared<T>(items...);
                NetObjID newID = NetObjID::random_gen();
                if(!objectData.emplace(newID, SingleObjectData{.netTypeID = isServer ? typeList->typeIndexDataServer[std::type_index(typeid(T*))].netTypeID : typeList->typeIndexDataClient[std::type_index(typeid(T*))].netTypeID, .p = std::static_pointer_cast<void>(sharedPtr)}).second)
                    throw std::runtime_error("[NetObjManager::make_obj] ID Collision");
                return NetObjPtr<T>(this, newID, sharedPtr);
            }
            template <typename T> NetObjPtr<T> obj_from_ptr(T* p) {
                std::shared_ptr<T> sharedPtr(p);
                NetObjID newID = NetObjID::random_gen();
                if(!objectData.emplace(newID, SingleObjectData{.netTypeID = isServer ? typeList->typeIndexDataServer[std::type_index(typeid(T*))].netTypeID : typeList->typeIndexDataClient[std::type_index(typeid(T*))].netTypeID, .p = std::static_pointer_cast<void>(sharedPtr)}).second)
                    throw std::runtime_error("[NetObjManager::obj_from_ptr] ID Collision");
                return NetObjPtr<T>(this, newID, sharedPtr);
            }
            template <typename T> NetObjPtr<T> get_obj_from_id(NetObjID id) {
                auto it = objectData.find(id);
                if(it == objectData.end())
                    throw std::runtime_error("[NetObjManager::get_obj_from_id] ID doesn't exist");
                return NetObjPtr<T>(this, id, std::static_pointer_cast<T>(it->second.p));
            }
        private:
            template <typename T> friend class NetObjPtr;

            struct SingleObjectData {
                NetTypeIDType netTypeID;
                std::shared_ptr<void> p;
            };
            bool isServer;
            std::shared_ptr<NetClient> client;
            std::shared_ptr<NetServer> server;
            MessageCommandType updateCommandID;
            std::unordered_map<NetObjID, SingleObjectData> objectData;
            std::shared_ptr<NetObjManagerTypeList> typeList;
            NetTypeIDType nextTypeID;
    };
}
