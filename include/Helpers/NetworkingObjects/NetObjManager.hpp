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
                a(id);
                NetObjPtr<T> newPtr = make_obj_with_id_might_already_exist<T>(id);
                typeList->get_type_index_data<T>(isServer).readConstructorFunc(newPtr.template cast<void>(), a, clientReceivedFrom);
                return newPtr;
            }
            template <typename T> NetObjPtr<T> read_get_obj_from_message(cereal::PortableBinaryInputArchive& a) {
                NetObjID id;
                a(id);
                auto it = objectData.find(id);
                if(it == objectData.end())
                    return NetObjPtr<T>();
                return NetObjPtr<T>(this, id, std::static_pointer_cast<T>(it->second.p));
            }
            template <typename T> NetObjPtr<T> make_obj() {
                return emplace_shared_ptr<T>(NetObjID::random_gen(), std::static_pointer_cast<T>(typeList->get_type_index_data<T>(isServer).allocatorFunc()));
            }
            // Don't use this function unless you're sure that class T isn't a base class
            template <typename T, typename... Args> NetObjPtr<T> make_obj_direct(Args&&... items) {
                return emplace_shared_ptr<T>(NetObjID::random_gen(), std::make_shared<T>(items...));
            }
            template <typename T> NetObjPtr<T> obj_from_ptr(T* p) {
                return emplace_shared_ptr<T>(NetObjID::random_gen(), std::shared_ptr<T>(p));
            }
            template <typename T> NetObjPtr<T> get_obj_from_id(NetObjID id) {
                auto it = objectData.find(id);
                if(it == objectData.end())
                    throw std::runtime_error("[NetObjManager::get_obj_from_id] ID doesn't exist");
                return NetObjPtr<T>(this, id, std::static_pointer_cast<T>(it->second.p));
            }
        private:
            template <typename T> NetObjPtr<T> make_obj_with_id_might_already_exist(NetObjID id) {
                auto alreadyExistsIt = objectData.find(id);
                if(alreadyExistsIt != objectData.end())
                    return NetObjPtr<T>(this, id, std::static_pointer_cast<T>(alreadyExistsIt->second.p));
                return emplace_shared_ptr<T>(id, std::static_pointer_cast<T>(typeList->get_type_index_data<T>(isServer).allocatorFunc()));
            }

            template <typename T> NetObjPtr<T> emplace_shared_ptr(NetObjID id, const std::shared_ptr<T>& sharedPtr) {
                if(!objectData.emplace(id, SingleObjectData{.netTypeID = typeList->get_type_index_data<T>(isServer).netTypeID, .p = std::static_pointer_cast<void>(sharedPtr)}).second)
                    throw std::runtime_error("[NetObjManager::emplace_shared_ptr] ID Collision");
                return NetObjPtr<T>(this, id, sharedPtr);
            }

            template <typename T> friend class NetObjPtr;
            template <typename T> friend class NetObjWeakPtr;

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
