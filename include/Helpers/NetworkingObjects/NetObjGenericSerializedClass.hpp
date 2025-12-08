#pragma once
#include "NetObjPtr.hpp"

namespace NetworkingObjects {
    template <typename T> void register_generic_serialized_class(NetObjManagerTypeList& t) {
        t.register_class<T, T>({
            .writeConstructorFuncClient = [](const NetObjPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                a(*o);
            },
            .readConstructorFuncClient = [](NetObjManager& m, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) {
                NetObjPtr<T> o = m.make_obj<T>();
                a(*o);
                return o.template cast<void>();
            },
            .readUpdateFuncClient = [](const NetObjPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) {
                a(*o);
            },
            .writeConstructorFuncServer = [](const NetObjPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                a(*o);
            },
            .readConstructorFuncServer = [](NetObjManager& m, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) {
                NetObjPtr<T> o = m.make_obj<T>();
                a(*o);
                return o.template cast<void>();
            },
            .readUpdateFuncServer = [](const NetObjPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) {
                a(*o);
                o.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [](const NetObjPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                    a(*o);
                });
            },
        });
    }
}
