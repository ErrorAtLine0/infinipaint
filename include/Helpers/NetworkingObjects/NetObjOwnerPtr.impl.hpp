#pragma once
#include <compare>
#include <string>
#include <functional>
#include <cereal/archives/portable_binary.hpp>
#include <type_traits>
#include "NetObjOwnerPtr.decl.hpp"
#include "NetObjTemporaryPtr.hpp"
#include "NetObjManager.hpp"

namespace NetworkingObjects {
    template <typename T> NetObjOwnerPtr<T>::NetObjOwnerPtr():
        objMan(nullptr),
        rawPtr(nullptr)
    {}

    template <typename T> NetObjOwnerPtr<T>::NetObjOwnerPtr(NetObjOwnerPtr&& other):
        objMan(other.objMan),
        id(other.id),
        rawPtr(other.rawPtr)
    {
        other.objMan = nullptr;
        other.rawPtr = nullptr;
    }

    template <typename T> NetObjOwnerPtr<T>& NetObjOwnerPtr<T>::operator=(NetObjOwnerPtr&& other) {
        objMan = other.objMan;
        id = other.id;
        rawPtr = other.rawPtr;
        other.objMan = nullptr;
        other.rawPtr = nullptr;
        return *this;
    }

    template <typename T> NetObjOwnerPtr<T>::~NetObjOwnerPtr() {
        if(objMan && rawPtr) {
            auto it = objMan->objectData.find(id);
            delete static_cast<T*>(it->second.p);
            objMan->objectData.erase(it);
        }
    }

    template <typename T> NetObjManager* NetObjOwnerPtr<T>::get_obj_man() const {
        return objMan;
    }

    template <typename T> NetObjID NetObjOwnerPtr<T>::get_net_id() const {
        return id;
    }

    template <typename T> T* NetObjOwnerPtr<T>::get() const {
        return rawPtr;
    }

    template <typename T> std::add_lvalue_reference_t<T> NetObjOwnerPtr<T>::operator*() const requires(!std::is_void_v<T>) {
        return *rawPtr;
    }

    template <typename T> T* NetObjOwnerPtr<T>::operator->() const {
        return rawPtr;
    }

    template <typename T> NetObjOwnerPtr<T>::operator bool() const {
        return rawPtr;
    }

    template <typename T> std::strong_ordering NetObjOwnerPtr<T>::operator<=>(const NetObjOwnerPtr<T>& rhs) const {
        return rawPtr <=> rhs.rawPtr;
    }

    template <typename T> void NetObjOwnerPtr<T>::send_client_update(const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const {
        NetObjManager::send_client_update(NetObjTemporaryPtr<T>(*this), channel, sendUpdateFunc);
    }

    template <typename T> void NetObjOwnerPtr<T>::send_server_update_to_all_clients(const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const {
        NetObjManager::send_server_update_to_all_clients(NetObjTemporaryPtr<T>(*this), channel, sendUpdateFunc);
    }

    template <typename T> void NetObjOwnerPtr<T>::send_server_update_to_client(const std::shared_ptr<NetServer::ClientData>& clientToSendTo, const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const {
        NetObjManager::send_server_update_to_client(NetObjTemporaryPtr<T>(*this), clientToSendTo, channel, sendUpdateFunc);
    }
    template <typename T> void NetObjOwnerPtr<T>::send_server_update_to_all_clients_except(const std::shared_ptr<NetServer::ClientData>& clientToNotSendTo, const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const {
        NetObjManager::send_server_update_to_all_clients_except(NetObjTemporaryPtr<T>(*this), clientToNotSendTo, channel, sendUpdateFunc);
    }

    template <typename T> void NetObjOwnerPtr<T>::send_update_to_all(const std::string& channel, std::function<void(const NetObjTemporaryPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const {
        NetObjManager::send_update_to_all(NetObjTemporaryPtr<T>(*this), channel, sendUpdateFunc);
    }

    template <typename T> void NetObjOwnerPtr<T>::write_create_message(cereal::PortableBinaryOutputArchive& a) const {
        NetObjManager::write_create_message(NetObjTemporaryPtr<T>(*this), a);
    }

    template <typename T> NetObjOwnerPtr<T>::NetObjOwnerPtr(NetObjManager* initObjMan, NetObjID initID, T* initRawPtr):
        objMan(initObjMan),
        id(initID),
        rawPtr(initRawPtr)
    {}
}
