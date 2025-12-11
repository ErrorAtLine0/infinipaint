#pragma once
#include "../Networking/NetLibrary.hpp"
#include <cereal/archives/portable_binary.hpp>
#include "NetObjTemporaryPtr.decl.hpp"
#include "../Networking/NetServer.hpp"

namespace NetworkingObjects {

class NetObjManagerTypeList {
    public:
        template <typename ClientT, typename ServerT> struct ServerClientClassFunctions {
            std::function<void(const NetObjTemporaryPtr<ClientT>&, cereal::PortableBinaryOutputArchive&)> writeConstructorFuncClient;
            std::function<void(const NetObjTemporaryPtr<ClientT>&, cereal::PortableBinaryInputArchive&, const std::shared_ptr<NetServer::ClientData>&)> readConstructorFuncClient;
            std::function<void(const NetObjTemporaryPtr<ClientT>&, cereal::PortableBinaryInputArchive&, const std::shared_ptr<NetServer::ClientData>&)> readUpdateFuncClient;

            std::function<void(const NetObjTemporaryPtr<ServerT>&, cereal::PortableBinaryOutputArchive&)> writeConstructorFuncServer;
            std::function<void(const NetObjTemporaryPtr<ServerT>&, cereal::PortableBinaryInputArchive&, const std::shared_ptr<NetServer::ClientData>&)> readConstructorFuncServer;
            std::function<void(const NetObjTemporaryPtr<ServerT>&, cereal::PortableBinaryInputArchive&, const std::shared_ptr<NetServer::ClientData>&)> readUpdateFuncServer;
        };
        template <typename ClientT, typename ServerT, typename ClientAllocatedType, typename ServerAllocatedType> void register_class(const ServerClientClassFunctions<ClientT, ServerT>& funcs) {
            typeIndexFunctionsClient[std::type_index(typeid(ClientT*))] = TypeIndexFunctions {
                .readConstructorFunc = [f = funcs.readConstructorFuncClient](const NetObjTemporaryPtr<void>& obj, cereal::PortableBinaryInputArchive& message, const std::shared_ptr<NetServer::ClientData>& c) {
                    if(f)
                        f(obj.cast<ClientT>(), message, c);
                },
                .writeConstructorFunc = [f = funcs.writeConstructorFuncClient](const NetObjTemporaryPtr<void>& obj, cereal::PortableBinaryOutputArchive& message) {
                    f(obj.cast<ClientT>(), message);
                },
                .allocatorFunc = []() { return static_cast<void*>(new ClientAllocatedType); },
                .netTypeID = netTypeID
            };

            typeIndexFunctionsServer[std::type_index(typeid(ServerT*))] = TypeIndexFunctions {
                .readConstructorFunc = [f = funcs.readConstructorFuncServer](const NetObjTemporaryPtr<void>& obj, cereal::PortableBinaryInputArchive& message, const std::shared_ptr<NetServer::ClientData>& c) {
                    if(f)
                        f(obj.cast<ServerT>(), message, c);
                },
                .writeConstructorFunc = [f = funcs.writeConstructorFuncServer](const NetObjTemporaryPtr<void>& obj, cereal::PortableBinaryOutputArchive& message) {
                    f(obj.cast<ServerT>(), message);
                },
                .allocatorFunc = []() { return static_cast<void*>(new ServerAllocatedType); },
                .netTypeID = netTypeID
            };

            netTypeIDFunctionsClient[netTypeID] = NetTypeIDFunctions {
                .readUpdateFunc = [f = funcs.readUpdateFuncClient](const NetObjTemporaryPtr<void>& obj, cereal::PortableBinaryInputArchive& message, const std::shared_ptr<NetServer::ClientData>& c) {
                    if(f)
                        f(obj.cast<ClientT>(), message, c);
                }
            };

            netTypeIDFunctionsServer[netTypeID] = NetTypeIDFunctions {
                .readUpdateFunc = [f = funcs.readUpdateFuncServer](const NetObjTemporaryPtr<void>& obj, cereal::PortableBinaryInputArchive& message, const std::shared_ptr<NetServer::ClientData>& c) {
                    if(f)
                        f(obj.cast<ServerT>(), message, c);
                }
            };

            netTypeID++;
        }

        struct TypeIndexFunctions {
            std::function<void(const NetObjTemporaryPtr<void>&, cereal::PortableBinaryInputArchive&, const std::shared_ptr<NetServer::ClientData>&)> readConstructorFunc;
            std::function<void(const NetObjTemporaryPtr<void>&, cereal::PortableBinaryOutputArchive&)> writeConstructorFunc;
            std::function<void*()> allocatorFunc;
            NetTypeIDType netTypeID;
        };

        struct NetTypeIDFunctions {
            std::function<void(const NetObjTemporaryPtr<void>&, cereal::PortableBinaryInputArchive&, const std::shared_ptr<NetServer::ClientData>&)> readUpdateFunc;
        };

        template <typename T> const TypeIndexFunctions& get_type_index_data(bool isServer) {
            return isServer ? typeIndexFunctionsServer[std::type_index(typeid(T*))] : typeIndexFunctionsClient[std::type_index(typeid(T*))];
        }

        const NetTypeIDFunctions& get_net_type_data(bool isServer, NetTypeIDType id);
    private:
        // std::type_index in this case is the typeinfo for the POINTER of the type in question. This ensures that we get a mapping for the exact type that was registered, not for a child of the type in case of inheritance
        std::unordered_map<std::type_index, TypeIndexFunctions> typeIndexFunctionsClient;
        std::unordered_map<std::type_index, TypeIndexFunctions> typeIndexFunctionsServer;
        std::unordered_map<NetTypeIDType, NetTypeIDFunctions> netTypeIDFunctionsClient;
        std::unordered_map<NetTypeIDType, NetTypeIDFunctions> netTypeIDFunctionsServer;

        NetTypeIDType netTypeID = 0;
};

}
