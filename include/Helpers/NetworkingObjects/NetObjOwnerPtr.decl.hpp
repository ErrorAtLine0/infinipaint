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
#include "NetObjID.hpp"
#include <type_traits>
#include <cereal/archives/portable_binary.hpp>
#include "../Networking/NetServer.hpp"

namespace NetworkingObjects {
    class NetObjManager;
    template <typename T> class NetObjTemporaryPtr;
    template <typename T> class NetObjOwnerPtr {
        public:
            NetObjOwnerPtr();
            NetObjOwnerPtr(const NetObjOwnerPtr& other) = delete;
            NetObjOwnerPtr(NetObjOwnerPtr&& other);
            NetObjOwnerPtr& operator=(const NetObjOwnerPtr& other) = delete;
            NetObjOwnerPtr& operator=(NetObjOwnerPtr&& other);
            ~NetObjOwnerPtr();
            NetObjManager* get_obj_man() const;
            NetObjID get_net_id() const;
            T* get() const;
            void reassign_ids();
            std::add_lvalue_reference_t<T> operator*() const requires(!std::is_void_v<T>);
            T* operator->() const;
            operator bool() const;
            bool operator!=(const NetObjOwnerPtr<T>& rhs) const;
            bool operator==(const NetObjOwnerPtr<T>& rhs) const;
            bool operator!=(const NetObjTemporaryPtr<T>& rhs) const;
            bool operator==(const NetObjTemporaryPtr<T>& rhs) const;
            void send_client_update(const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const;
            void send_server_update_to_client(const std::shared_ptr<NetServer::ClientData>& clientToSendTo, const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const;
            void send_server_update_to_all_clients_except(const std::shared_ptr<NetServer::ClientData>& clientToNotSendTo, const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const;
            void send_server_update_to_all_clients(const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const;
            void send_update_to_all(const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const;
            void write_create_message(cereal::PortableBinaryOutputArchive& a) const;
        private:
            friend class NetObjManager;
            template <typename S> friend class NetObjWeakPtr;
            template <typename S> friend class NetObjTemporaryPtr;

            NetObjOwnerPtr(NetObjManager* initObjMan, NetObjID initID, T* initRawPtr);
            NetObjManager* objMan;
            NetObjID id;
            T* rawPtr;
    };
}
