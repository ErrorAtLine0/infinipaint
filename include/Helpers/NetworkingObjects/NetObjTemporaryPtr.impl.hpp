#pragma once
#include <compare>
#include <string>
#include <functional>
#include <cereal/archives/portable_binary.hpp>
#include <type_traits>
#include "NetObjTemporaryPtr.decl.hpp"
#include "NetObjManager.hpp"

namespace NetworkingObjects {
    template <typename T> NetObjTemporaryPtr<T>::NetObjTemporaryPtr():
        objMan(nullptr),
        rawPtr(nullptr)
    {}
    template <typename T> NetObjTemporaryPtr<T>::NetObjTemporaryPtr(const NetObjTemporaryPtr<T>& other):
        objMan(other.objMan),
        id(other.id),
        rawPtr(other.rawPtr)
    {}
    template <typename T> NetObjTemporaryPtr<T>::NetObjTemporaryPtr(const NetObjOwnerPtr<T>& owner):
        objMan(owner.objMan),
        id(owner.id),
        rawPtr(owner.rawPtr)
    {}
    
    template <typename T> NetObjTemporaryPtr<T>& NetObjTemporaryPtr<T>::operator=(const NetObjTemporaryPtr<T>& other) {
        objMan = other.objMan;
        id = other.id;
        rawPtr = other.rawPtr;
        return *this;
    }
    template <typename T> template <typename S> NetObjTemporaryPtr<S> NetObjTemporaryPtr<T>::cast() const {
        return NetObjTemporaryPtr<S>(objMan, id, static_cast<S*>(rawPtr));
    }

    template <typename T> NetObjManager* NetObjTemporaryPtr<T>::get_obj_man() const {
        return objMan;
    }

    template <typename T> NetObjID NetObjTemporaryPtr<T>::get_net_id() const {
        return id;
    }

    template <typename T> T* NetObjTemporaryPtr<T>::get() const {
        return rawPtr;
    }

    template <typename T> std::add_lvalue_reference_t<T> NetObjTemporaryPtr<T>::operator*() const requires(!std::is_void_v<T>) {
        return *rawPtr;
    }

    template <typename T> T* NetObjTemporaryPtr<T>::operator->() const {
        return rawPtr;
    }

    template <typename T> NetObjTemporaryPtr<T>::operator bool() const {
        return rawPtr;
    }

    template <typename T> bool NetObjTemporaryPtr<T>::operator!=(const NetObjTemporaryPtr<T>& rhs) const {
        return rawPtr != rhs.rawPtr;
    }

    template <typename T> bool NetObjTemporaryPtr<T>::operator==(const NetObjTemporaryPtr<T>& rhs) const {
        return rawPtr == rhs.rawPtr;
    }

    template <typename T> bool NetObjTemporaryPtr<T>::operator!=(const NetObjOwnerPtr<T>& rhs) const {
        return rawPtr != rhs.get();
    }

    template <typename T> bool NetObjTemporaryPtr<T>::operator==(const NetObjOwnerPtr<T>& rhs) const {
        return rawPtr == rhs.get();
    }

    template <typename T> void NetObjTemporaryPtr<T>::send_client_update(const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const {
        NetObjManager::send_client_update(*this, channel, sendUpdateFunc);
    }

    template <typename T> void NetObjTemporaryPtr<T>::send_server_update_to_all_clients(const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const {
        NetObjManager::send_server_update_to_all_clients(*this, channel, sendUpdateFunc);
    }


    template <typename T> void NetObjTemporaryPtr<T>::send_server_update_to_client(const std::shared_ptr<NetServer::ClientData>& clientToSendTo, const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const {
        NetObjManager::send_server_update_to_client(*this, clientToSendTo, channel, sendUpdateFunc);
    }
    template <typename T> void NetObjTemporaryPtr<T>::send_server_update_to_all_clients_except(const std::shared_ptr<NetServer::ClientData>& clientToNotSendTo, const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const {
        NetObjManager::send_server_update_to_all_clients_except(*this, clientToNotSendTo, channel, sendUpdateFunc);
    }

    template <typename T> void NetObjTemporaryPtr<T>::send_update_to_all(const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const {
        NetObjManager::send_update_to_all(*this, channel, sendUpdateFunc);
    }

    template <typename T> void NetObjTemporaryPtr<T>::write_create_message(cereal::PortableBinaryOutputArchive& a) const {
        NetObjManager::write_create_message(*this, a);
    }

    template <typename T> NetObjTemporaryPtr<T>::NetObjTemporaryPtr(NetObjManager* initObjMan, NetObjID initID, T* initRawPtr):
        objMan(initObjMan),
        id(initID),
        rawPtr(initRawPtr)
    {}
}

