#pragma once
#include "../Networking/NetLibrary.hpp"
#include <cereal/archives/portable_binary.hpp>
#include "NetObjPtr.decl.hpp"
#include "../Networking/NetServer.hpp"

namespace NetworkingObjects {

class NetObjManagerTypeList {
    public:
        template <typename ClientT, typename ServerT> struct ServerClientClassFunctions {
            std::function<void(const NetObjPtr<ClientT>&, cereal::PortableBinaryOutputArchive&)> writeConstructorFuncClient;
            std::function<void(const NetObjPtr<ClientT>&, cereal::PortableBinaryInputArchive&, const std::shared_ptr<NetServer::ClientData>&)> readConstructorFuncClient;
            std::function<void(const NetObjPtr<ClientT>&, cereal::PortableBinaryInputArchive&, const std::shared_ptr<NetServer::ClientData>&)> readUpdateFuncClient;

            std::function<void(const NetObjPtr<ServerT>&, cereal::PortableBinaryOutputArchive&)> writeConstructorFuncServer;
            std::function<void(const NetObjPtr<ServerT>&, cereal::PortableBinaryInputArchive&, const std::shared_ptr<NetServer::ClientData>&)> readConstructorFuncServer;
            std::function<void(const NetObjPtr<ServerT>&, cereal::PortableBinaryInputArchive&, const std::shared_ptr<NetServer::ClientData>&)> readUpdateFuncServer;
        };
        template <typename ClientT, typename ServerT, typename ClientAllocatedType, typename ServerAllocatedType> void register_class(const ServerClientClassFunctions<ClientT, ServerT>& funcs) {
            netTypeIDData[nextNetTypeID] = NetTypeIDContainer {
                .readConstructorFuncClient = [f = funcs.readConstructorFuncClient](const NetObjPtr<void>& obj, cereal::PortableBinaryInputArchive& message, const std::shared_ptr<NetServer::ClientData>& c) {
                    if(f)
                        f(obj.cast<ClientT>(), message, c);
                },
                .readUpdateFuncClient = [f = funcs.readUpdateFuncClient](const NetObjPtr<void>& obj, cereal::PortableBinaryInputArchive& message, const std::shared_ptr<NetServer::ClientData>& c) {
                    if(f)
                        f(obj.cast<ClientT>(), message, c);
                },
                .readConstructorFuncServer = [f = funcs.readConstructorFuncServer](const NetObjPtr<void>& obj, cereal::PortableBinaryInputArchive& message, const std::shared_ptr<NetServer::ClientData>& c) {
                    if(f)
                        f(obj.cast<ServerT>(), message, c);
                },
                .readUpdateFuncServer = [f = funcs.readUpdateFuncServer](const NetObjPtr<void>& obj, cereal::PortableBinaryInputArchive& message, const std::shared_ptr<NetServer::ClientData>& c) {
                    if(f)
                        f(obj.cast<ServerT>(), message, c);
                },
            };
            typeIndexDataClient[std::type_index(typeid(ClientT*))] = TypeIndexContainer{
                .netTypeID = nextNetTypeID,
                .writeConstructorFunc = [f = funcs.writeConstructorFuncClient](const NetObjPtr<void>& obj, cereal::PortableBinaryOutputArchive& message) {
                    f(obj.cast<ClientT>(), message);
                },
                .allocatorFunc = []() { return std::make_shared<ClientAllocatedType>(); }
            };
            typeIndexDataServer[std::type_index(typeid(ServerT*))] = TypeIndexContainer{
                .netTypeID = nextNetTypeID,
                .writeConstructorFunc = [f = funcs.writeConstructorFuncServer](const NetObjPtr<void>& obj, cereal::PortableBinaryOutputArchive& message) {
                    f(obj.cast<ServerT>(), message);
                },
                .allocatorFunc = []() { return std::make_shared<ServerAllocatedType>(); }
            };
            nextNetTypeID++;
        }
    private:
        friend class NetObjManager;
        template <typename T> friend class NetObjPtr;

        struct TypeIndexContainer {
            NetTypeIDType netTypeID;
            std::function<void(const NetObjPtr<void>&, cereal::PortableBinaryOutputArchive&)> writeConstructorFunc;
            std::function<std::shared_ptr<void>()> allocatorFunc;
        };

        // std::type_index in this case is the typeinfo for the POINTER of the type in question. This ensures that we get a mapping for the exact type that was registered, not for a child of the type in case of inheritance
        std::unordered_map<std::type_index, TypeIndexContainer> typeIndexDataClient;
        std::unordered_map<std::type_index, TypeIndexContainer> typeIndexDataServer;

        struct NetTypeIDContainer {
            std::function<void(const NetObjPtr<void>&, cereal::PortableBinaryInputArchive&, const std::shared_ptr<NetServer::ClientData>&)> readConstructorFuncClient;
            std::function<void(const NetObjPtr<void>&, cereal::PortableBinaryInputArchive&, const std::shared_ptr<NetServer::ClientData>&)> readUpdateFuncClient;

            std::function<void(const NetObjPtr<void>&, cereal::PortableBinaryInputArchive&, const std::shared_ptr<NetServer::ClientData>&)> readConstructorFuncServer;
            std::function<void(const NetObjPtr<void>&, cereal::PortableBinaryInputArchive&, const std::shared_ptr<NetServer::ClientData>&)> readUpdateFuncServer;
        };
        std::unordered_map<NetTypeIDType, NetTypeIDContainer> netTypeIDData;

        NetTypeIDType nextNetTypeID = 0;
};

}
