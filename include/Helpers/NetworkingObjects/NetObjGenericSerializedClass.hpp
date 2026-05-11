/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include "NetObjTemporaryPtr.hpp"
#include "cereal/archives/portable_binary.hpp"
#include <chrono>

namespace NetworkingObjects {
    template <typename T> void default_serialize_write_func(const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
        a(*o);
    }

    template <typename T> void default_serialize_read_func_client(const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) {
        a(*o);
    }

    template <typename T> void default_serialize_read_constructor_func(const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) {
        a(*o);
    }

    template <typename T> void register_generic_serialized_class(NetObjManager& objMan) {
        objMan.register_class<T, T, T, T>({
            .writeConstructorFuncClient = default_serialize_write_func<T>,
            .readConstructorFuncClient = default_serialize_read_constructor_func<T>,
            .readUpdateFuncClient = default_serialize_read_func_client<T>,
            .writeConstructorFuncServer = default_serialize_write_func<T>,
            .readConstructorFuncServer = default_serialize_read_constructor_func<T>,
            .readUpdateFuncServer = [](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) {
                a(*o);
                o.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                    a(*o);
                });
            },
        });
    }

    template <typename T> void generic_serialized_class_send_update_to_all(const NetObjTemporaryPtr<T>& o) {
        o.send_update_to_all(RELIABLE_COMMAND_CHANNEL, default_serialize_write_func<T>);
    }
}
