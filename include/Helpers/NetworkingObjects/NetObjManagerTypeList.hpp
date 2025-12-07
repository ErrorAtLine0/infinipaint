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
        template <typename ClientPointerType, typename ServerPointerType, typename ClientActualType, typename ServerActualType> void register_class(const ServerClientClassFunctions<ClientPointerType, ServerPointerType>& funcs) {
            netTypeIDData[nextNetTypeID] = NetTypeIDContainer {
                .readConstructorFuncClient = [f = funcs.readConstructorFuncClient](const NetObjPtr<void>& obj, cereal::PortableBinaryInputArchive& message, const std::shared_ptr<NetServer::ClientData>& c) {
                    if(f)
                        f(obj.cast<ClientPointerType>(), message, c);
                },
                .readUpdateFuncClient = [f = funcs.readUpdateFuncClient](const NetObjPtr<void>& obj, cereal::PortableBinaryInputArchive& message, const std::shared_ptr<NetServer::ClientData>& c) {
                    if(f)
                        f(obj.cast<ClientPointerType>(), message, c);
                },
                .allocatorFuncClient = []() { return std::static_pointer_cast<void>(std::make_shared<ClientActualType>()); },

                .readConstructorFuncServer = [f = funcs.readConstructorFuncServer](const NetObjPtr<void>& obj, cereal::PortableBinaryInputArchive& message, const std::shared_ptr<NetServer::ClientData>& c) {
                    if(f)
                        f(obj.cast<ServerPointerType>(), message, c);
                },
                .readUpdateFuncServer = [f = funcs.readUpdateFuncServer](const NetObjPtr<void>& obj, cereal::PortableBinaryInputArchive& message, const std::shared_ptr<NetServer::ClientData>& c) {
                    if(f)
                        f(obj.cast<ServerPointerType>(), message, c);
                },
                .allocatorFuncServer = []() { return std::static_pointer_cast<void>(std::make_shared<ServerActualType>()); }
            };
            typeIndexDataClient[std::type_index(typeid(ClientActualType))] = TypeIndexContainer{
                .netTypeID = nextNetTypeID,
                .writeConstructorFunc = [f = funcs.writeConstructorFuncClient](const NetObjPtr<void>& obj, cereal::PortableBinaryOutputArchive& message) {
                    if(f)
                        f(obj.cast<ClientPointerType>(), message);
                },
            };
            typeIndexDataServer[std::type_index(typeid(ServerActualType))] = TypeIndexContainer{
                .netTypeID = nextNetTypeID,
                .writeConstructorFunc = [f = funcs.writeConstructorFuncServer](const NetObjPtr<void>& obj, cereal::PortableBinaryOutputArchive& message) {
                    if(f)
                        f(obj.cast<ServerPointerType>(), message);
                },
            };
            nextNetTypeID++;
        }
    private:
        friend class NetObjManager;
        template <typename T> friend class NetObjPtr;

        struct TypeIndexContainer {
            NetTypeIDType netTypeID;
            std::function<void(const NetObjPtr<void>&, cereal::PortableBinaryOutputArchive&)> writeConstructorFunc;
        };

        std::unordered_map<std::type_index, TypeIndexContainer> typeIndexDataClient;
        std::unordered_map<std::type_index, TypeIndexContainer> typeIndexDataServer;

        struct NetTypeIDContainer {
            std::function<void(const NetObjPtr<void>&, cereal::PortableBinaryInputArchive&, const std::shared_ptr<NetServer::ClientData>&)> readConstructorFuncClient;
            std::function<void(const NetObjPtr<void>&, cereal::PortableBinaryInputArchive&, const std::shared_ptr<NetServer::ClientData>&)> readUpdateFuncClient;
            std::function<std::shared_ptr<void>()> allocatorFuncClient;

            std::function<void(const NetObjPtr<void>&, cereal::PortableBinaryInputArchive&, const std::shared_ptr<NetServer::ClientData>&)> readConstructorFuncServer;
            std::function<void(const NetObjPtr<void>&, cereal::PortableBinaryInputArchive&, const std::shared_ptr<NetServer::ClientData>&)> readUpdateFuncServer;
            std::function<std::shared_ptr<void>()> allocatorFuncServer;
        };
        std::unordered_map<NetTypeIDType, NetTypeIDContainer> netTypeIDData;

        NetTypeIDType nextNetTypeID = 0;
};

}
