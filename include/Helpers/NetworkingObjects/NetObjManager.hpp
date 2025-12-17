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
#include "NetObjOwnerPtr.decl.hpp"
#include "NetObjTemporaryPtr.decl.hpp"
#include "../Networking/NetServer.hpp"

namespace NetworkingObjects {
    class NetObjManager {
        public:
            NetObjManager(bool initIsServer);
            void read_update_message(cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& clientReceivedFrom);
            void set_client(std::shared_ptr<NetClient> initClient, MessageCommandType initUpdateCommandID);
            void set_server(std::shared_ptr<NetServer> initServer, MessageCommandType initUpdateCommandID);
            void disconnect();
            bool is_server() const;
            template <typename T> NetObjOwnerPtr<T> read_create_message(cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& clientReceivedFrom) {
                NetObjID id;
                a(id);
                auto it = objectData.find(id);
                if(it != objectData.end())
                    throw std::runtime_error("[NetObjManager::read_create_message] Attempted to create an object with a used ID");
                NetObjOwnerPtr<T> newPtr = emplace_raw_ptr(id, static_cast<T*>(typeList.get_type_index_data<T>(isServer).allocatorFunc()));
                typeList.get_type_index_data<T>(isServer).readConstructorFunc(NetObjTemporaryPtr(newPtr).template cast<void>(), a, clientReceivedFrom);
                return newPtr;
            }
            template <typename T> NetObjTemporaryPtr<T> read_get_obj_ref_from_message(cereal::PortableBinaryInputArchive& a) {
                NetObjID id;
                a(id);
                auto it = objectData.find(id);
                if(it == objectData.end())
                    return NetObjTemporaryPtr<T>();
                return NetObjTemporaryPtr<T>(this, id, std::static_pointer_cast<T>(it->second.p));
            }

            // NOTE: DO NOT USE THESE FUNCTIONS UNLESS SITUATION ISN'T NORMAL, EVERY OBJECT SHOULD HAVE A UNIQUE ID AND IDS SHOULDNT BE REUSED UNDER ANY CIRCUMSTANCES
            template <typename T> NetObjOwnerPtr<T> make_obj_with_specific_id(const NetObjID& id) {
                return emplace_raw_ptr<T>(id, static_cast<T*>(typeList.get_type_index_data<T>(isServer).allocatorFunc()));
            }
            template <typename T, typename... Args> NetObjOwnerPtr<T> make_obj_direct_with_specific_id(const NetObjID& id, Args&&... items) {
                return emplace_raw_ptr<T>(id, new T(items...));
            }

            template <typename T> NetObjOwnerPtr<T> make_obj() {
                return emplace_raw_ptr<T>(NetObjID::random_gen(), static_cast<T*>(typeList.get_type_index_data<T>(isServer).allocatorFunc()));
            }
            // Don't use this function unless you're sure that class T isn't a base class
            template <typename T, typename... Args> NetObjOwnerPtr<T> make_obj_direct(Args&&... items) {
                return emplace_raw_ptr<T>(NetObjID::random_gen(), new T(items...));
            }
            template <typename T> NetObjOwnerPtr<T> make_obj_from_ptr(T* p) {
                return emplace_raw_ptr<T>(NetObjID::random_gen(), p);
            }
            template <typename T> NetObjOwnerPtr<T> get_obj_from_id(NetObjID id) {
                auto it = objectData.find(id);
                if(it == objectData.end())
                    throw std::runtime_error("[NetObjManager::get_obj_from_id] ID doesn't exist");
                return NetObjOwnerPtr<T>(this, id, static_cast<T*>(it->second.p));
            }
            template <typename T> NetObjTemporaryPtr<T> get_obj_temporary_ref_from_id(NetObjID id) {
                auto it = objectData.find(id);
                if(it == objectData.end())
                    return NetObjTemporaryPtr<T>();
                return NetObjTemporaryPtr<T>(this, id, static_cast<T*>(it->second.p));
            }
            template <typename ClientT, typename ServerT, typename ClientAllocatedType, typename ServerAllocatedType> void register_class(const NetObjManagerTypeList::ServerClientClassFunctions<ClientT, ServerT>& funcs) {
                typeList.register_class<ClientT, ServerT, ClientAllocatedType, ServerAllocatedType>(funcs);
            }
        private:
            template <typename T> NetObjOwnerPtr<T> emplace_raw_ptr(NetObjID id, T* rawPtr) {
                if(!objectData.emplace(id, SingleObjectData{.netTypeID = typeList.get_type_index_data<T>(isServer).netTypeID, .p = rawPtr}).second)
                    throw std::runtime_error("[NetObjManager::emplace_raw_ptr] ID Collision");
                return NetObjOwnerPtr<T>(this, id, rawPtr);
            }

            template <typename T> static void send_update_to_all(const NetObjTemporaryPtr<T>& ptr, const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) {
                if(ptr.get_obj_man()->isServer)
                    NetObjManager::send_server_update_to_all_clients(ptr, channel, sendUpdateFunc);
                else
                    NetObjManager::send_client_update(ptr, channel, sendUpdateFunc);
            }

            template <typename T> static void send_client_update(const NetObjTemporaryPtr<T>& ptr, const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) {
                if(ptr.get_obj_man()->client && !ptr.get_obj_man()->client->is_disconnected()) {
                    auto ss(std::make_shared<std::stringstream>());
                    {
                        cereal::PortableBinaryOutputArchive m(*ss);
                        m(ptr.get_obj_man()->updateCommandID, ptr.get_net_id());
                        sendUpdateFunc(ptr, m);
                    }
                    ptr.get_obj_man()->client->send_string_stream_to_server(channel, ss);
                }
            }

            template <typename T> static void send_server_update_to_client(const NetObjTemporaryPtr<T>& ptr, const std::shared_ptr<NetServer::ClientData>& clientToSendTo, const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) {
                if(ptr.get_obj_man()->server && !ptr.get_obj_man()->server->is_disconnected()) {
                    auto ss(std::make_shared<std::stringstream>());
                    {
                        cereal::PortableBinaryOutputArchive m(*ss);
                        m(ptr.get_obj_man()->updateCommandID, ptr.get_net_id());
                        sendUpdateFunc(ptr, m);
                    }
                    ptr.get_obj_man()->server->send_string_stream_to_client(clientToSendTo, channel, ss);
                }
            }

            template <typename T> static void send_server_update_to_all_clients_except(const NetObjTemporaryPtr<T>& ptr, const std::shared_ptr<NetServer::ClientData>& clientToNotSendTo, const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) {
                if(ptr.get_obj_man()->server && !ptr.get_obj_man()->server->is_disconnected()) {
                    auto ss(std::make_shared<std::stringstream>());
                    {
                        cereal::PortableBinaryOutputArchive m(*ss);
                        m(ptr.get_obj_man()->updateCommandID, ptr.get_net_id());
                        sendUpdateFunc(ptr, m);
                    }
                    ptr.get_obj_man()->server->send_string_stream_to_all_clients_except(clientToNotSendTo, channel, ss);
                }
            }

            template <typename T> static void send_server_update_to_all_clients(const NetObjTemporaryPtr<T>& ptr, const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) {
                if(ptr.get_obj_man()->server && !ptr.get_obj_man()->server->is_disconnected()) {
                    auto ss(std::make_shared<std::stringstream>());
                    {
                        cereal::PortableBinaryOutputArchive m(*ss);
                        m(ptr.get_obj_man()->updateCommandID, ptr.get_net_id());
                        sendUpdateFunc(ptr, m);
                    }
                    ptr.get_obj_man()->server->send_string_stream_to_all_clients(channel, ss);
                }
            }

            template <typename T> static void write_create_message(const NetObjTemporaryPtr<T>& ptr, cereal::PortableBinaryOutputArchive& a) {
                a(ptr.get_net_id());
                ptr.get_obj_man()->typeList.template get_type_index_data<T>(ptr.get_obj_man()->isServer).writeConstructorFunc(ptr.template cast<void>(), a);
            }

            template <typename T> friend class NetObjOwnerPtr;
            template <typename T> friend class NetObjTemporaryPtr;
            template <typename T> friend class NetObjWeakPtr;

            struct SingleObjectData {
                NetTypeIDType netTypeID;
                void* p;
            };

            bool isServer;
            std::shared_ptr<NetClient> client;
            std::shared_ptr<NetServer> server;
            MessageCommandType updateCommandID;
            std::unordered_map<NetObjID, SingleObjectData> objectData;
            NetObjManagerTypeList typeList;
            NetTypeIDType nextTypeID;
    };
}
