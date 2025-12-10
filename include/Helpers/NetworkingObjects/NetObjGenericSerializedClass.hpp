#pragma once
#include "NetObjPtr.hpp"
#include "cereal/archives/portable_binary.hpp"
#include <chrono>

namespace NetworkingObjects {
    template <typename T> void default_serialize_write_func(const NetObjPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
        a(*o);
    }

    template <typename T> void default_serialize_read_func(const NetObjPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) {
        a(*o);
    }

    template <typename T> void register_generic_serialized_class(NetObjManagerTypeList& t) {
        t.register_class<T, T, T, T>({
            .writeConstructorFuncClient = default_serialize_write_func<T>,
            .readConstructorFuncClient = default_serialize_read_func<T>,
            .readUpdateFuncClient = default_serialize_read_func<T>,
            .writeConstructorFuncServer = default_serialize_write_func<T>,
            .readConstructorFuncServer = default_serialize_read_func<T>,
            .readUpdateFuncServer = [](const NetObjPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) {
                a(*o);
                o.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [](const NetObjPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                    a(*o);
                });
            },
        });
    }

    template <typename T> void generic_serialized_class_send_update_to_all(const NetworkingObjects::NetObjPtr<T>& o) {
        o.send_update_to_all(RELIABLE_COMMAND_CHANNEL, default_serialize_write_func<T>);
    }
}
