#pragma once
#include "Helpers/NetworkingObjects/NetObjPtr.decl.hpp"
#include "NetObjPtr.hpp"

namespace NetworkingObjects {
    template <typename T> void register_generic_serialized_class(NetObjManagerTypeList& t) {
        t.register_class<T, T, T, T>({
            .writeConstructorFuncClient = [](const NetObjPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                a(*o);
            },
            .readConstructorFuncClient = [](const NetObjPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) {
                a(*o);
            },
            .readUpdateFuncClient = [](const NetObjPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) {
                a(*o);
            },
            .writeConstructorFuncServer = [](const NetObjPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                a(*o);
            },
            .readConstructorFuncServer = [](const NetObjPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) {
                a(*o);
            },
            .readUpdateFuncServer = [](const NetObjPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) {
                a(*o);
                o.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [](const NetObjPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                    a(*o);
                });
            },
        });
    }

    template <typename T> void generic_serialized_class_send_update_to_all(const NetworkingObjects::NetObjPtr<T>& o) {
        o.send_update_to_all(RELIABLE_COMMAND_CHANNEL, [](const NetworkingObjects::NetObjPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
            a(*o);
        });
    }
}
