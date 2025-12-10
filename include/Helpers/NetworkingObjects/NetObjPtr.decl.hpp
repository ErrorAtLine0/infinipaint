#pragma once
#include "NetObjID.hpp"
#include <type_traits>
#include <cereal/archives/portable_binary.hpp>

namespace NetworkingObjects {
    class NetObjManager;
    template <typename T> class NetObjPtr {
        public:
            NetObjPtr();
            NetObjPtr(const NetObjPtr& other);
            NetObjPtr& operator=(const NetObjPtr& other);
            ~NetObjPtr();
            template <typename S> NetObjPtr<S> cast() const;
            NetObjManager* get_obj_man() const;
            NetObjID get_net_id() const;
            T* get() const;
            std::add_lvalue_reference_t<T> operator*() const requires(!std::is_void_v<T>);
            T* operator->() const;
            operator bool() const;
            std::strong_ordering operator<=>(const NetObjPtr<T>& rhs) const;
            void send_client_update(const std::string& channel, std::function<void(const NetObjPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const;
            void send_server_update_to_all_clients(const std::string& channel, std::function<void(const NetObjPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const;
            void send_update_to_all(const std::string& channel, std::function<void(const NetObjPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const;
            void write_create_message(cereal::PortableBinaryOutputArchive& a) const;
        private:
            friend class NetObjManager;
            template <typename S> friend class NetObjPtr;
            template <typename S> friend class NetObjWeakPtr;

            NetObjPtr(NetObjManager* initObjMan, NetObjID initID, const std::shared_ptr<T>& initPtr);
            NetObjManager* objMan;
            NetObjID id;
            std::shared_ptr<T> sharedPtr;
    };
}
