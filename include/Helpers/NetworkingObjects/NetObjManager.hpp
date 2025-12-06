#pragma once
#include <compare>
#include <memory>
#include <unordered_map>
#include <functional>
#include <cereal/archives/portable_binary.hpp>
#include <chrono>
#include <vector>
#include "../Networking/NetClient.hpp"
#include "../Networking/NetServer.hpp"
#include "../Networking/NetLibrary.hpp"
#include "NetObjIDGenerator.hpp"
#include "NetObjManagerTypeList.hpp"

namespace NetworkingObjects {
    template <typename T> class NetObjPtr;

    class NetObjManager {
        public:
            NetObjManager(std::shared_ptr<NetObjIDGenerator> initIDGen, std::shared_ptr<NetObjManagerTypeList> initTypeList);
            void read_update_message(cereal::PortableBinaryInputArchive& a);
            void set_client(std::shared_ptr<NetClient> initClient, MessageCommandType initUpdateCommandID);
            void set_server(std::shared_ptr<NetServer> initServer, MessageCommandType initUpdateCommandID);

            template <typename T> NetObjPtr<T> read_create_message(cereal::PortableBinaryInputArchive& a) {
                NetObjID id;
                NetTypeIDType typeID;
                a(id, typeID);
                auto& netTypeIDData = typeList->netTypeIDData[typeID];
                std::shared_ptr<void> sharedPtr = netTypeIDData.allocatorFunc();
                auto [it, placed] = objectData.emplace(id, typeID, sharedPtr);
                if(!placed)
                    throw std::runtime_error("[NetObjManager::read_create_message] ID Collision on object creation");
                netTypeIDData.readConstructorFunc(sharedPtr.get(), a);
                return NetObjPtr<T>(id, std::static_pointer_cast<T>(sharedPtr));
            }
            template <typename T> NetObjPtr<T> read_get_obj_from_message(cereal::PortableBinaryInputArchive& a) {
                NetObjID id;
                a(id);
                auto it = objectData.find(id);
                if(it == objectData.end())
                    return NetObjPtr<T>();
                return NetObjPtr<T>(id, std::static_pointer_cast<T>(it->second.p));
            }
            template <typename T, typename... Args> NetObjPtr<T> make_obj(Args&&... items) {
                auto sharedPtr = std::make_shared<T>(items...);
                NetObjID newID = idGen->gen();
                if(!objectData.emplace(newID, typeList->typeIndexData[std::type_index(typeid(T))].netTypeID, sharedPtr).second)
                    throw std::runtime_error("[NetObjManager::make_obj] ID Collision");
                return NetObjPtr<T>(newID, sharedPtr);
            }
            template <typename T> NetObjPtr<T> obj_from_ptr(T* p) {
                std::shared_ptr<T> sharedPtr(p);
                NetObjID newID = idGen->gen();
                if(!objectData.emplace(newID, typeList->typeIndexData[std::type_index(typeid(T))].netTypeID, sharedPtr).second)
                    throw std::runtime_error("[NetObjManager::obj_from_ptr] ID Collision");
                return NetObjPtr<T>(newID, sharedPtr);
            }
            template <typename T> NetObjPtr<T> get_obj_from_id(NetObjID id) {
                auto it = objectData.find(id);
                if(it == objectData.end())
                    throw std::runtime_error("[NetObjManager::get_obj_from_id] ID doesn't exist");
                return NetObjPtr<T>(id, std::static_pointer_cast<T>(it->second.p));
            }
        private:
            template <typename T> friend class NetObjPtr;

            struct SingleObjectData {
                NetTypeIDType netTypeID;
                std::shared_ptr<void> p;
                std::optional<std::chrono::time_point<std::chrono::steady_clock>> timeSinceNoReference;
            };
            std::shared_ptr<NetClient> client;
            std::shared_ptr<NetServer> server;
            MessageCommandType updateCommandID;
            std::unordered_map<NetObjID, SingleObjectData> objectData;
            std::shared_ptr<NetObjIDGenerator> idGen;
            std::shared_ptr<NetObjManagerTypeList> typeList;
            NetTypeIDType nextTypeID;
    };
}
