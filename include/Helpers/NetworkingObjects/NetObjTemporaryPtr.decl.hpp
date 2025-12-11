#pragma once
#include "NetObjID.hpp"
#include <type_traits>
#include <cereal/archives/portable_binary.hpp>
#include "NetObjOwnerPtr.decl.hpp"
#include "../Networking/NetServer.hpp"

namespace NetworkingObjects {
    class NetObjManager;
    template <typename T> class NetObjTemporaryPtr {
        public:
            NetObjTemporaryPtr();
            NetObjTemporaryPtr(const NetObjTemporaryPtr& other);
            NetObjTemporaryPtr(const NetObjOwnerPtr<T>& owner);
            NetObjTemporaryPtr& operator=(const NetObjTemporaryPtr& other);
            template <typename S> NetObjTemporaryPtr<S> cast() const;
            NetObjManager* get_obj_man() const;
            NetObjID get_net_id() const;
            T* get() const;
            std::add_lvalue_reference_t<T> operator*() const requires(!std::is_void_v<T>);
            T* operator->() const;
            operator bool() const;
            std::strong_ordering operator<=>(const NetObjTemporaryPtr<T>& rhs) const;
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
            NetObjTemporaryPtr(NetObjManager* initObjMan, NetObjID initID, T* initRawPtr);

            NetObjManager* objMan;
            NetObjID id;
            T* rawPtr;
    };
}
