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
