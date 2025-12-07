#pragma once
#include <compare>
#include <string>
#include <functional>
#include <cereal/archives/portable_binary.hpp>
#include <type_traits>
#include "NetObjPtr.decl.hpp"
#include "NetObjManager.hpp"
#include "NetObjManagerTypeList.hpp"

namespace NetworkingObjects {
    template <typename T> NetObjPtr<T>::NetObjPtr():
        objMan(nullptr),
        sharedPtr(nullptr)
    {}
    template <typename T> NetObjPtr<T>::NetObjPtr(const NetObjPtr<T>& other):
        objMan(other.objMan),
        id(other.id),
        sharedPtr(other.sharedPtr)
    {}
    template <typename T> NetObjPtr<T>& NetObjPtr<T>::operator=(const NetObjPtr<T>& other) {
        if(this != &other) {
            if(sharedPtr && sharedPtr.unique())
                objMan->objectData.erase(id);
            objMan = other.objMan;
            id = other.id;
            sharedPtr = other.sharedPtr;
        }
        return *this;
    }
    template <typename T> NetObjPtr<T>::~NetObjPtr() {
        if(sharedPtr && sharedPtr.unique())
            objMan->objectData.erase(id);
    }

    template <typename T> template <typename S> NetObjPtr<S> NetObjPtr<T>::cast() const {
        return NetObjPtr<S>(objMan, id, std::static_pointer_cast<S>(sharedPtr));
    }

    template <typename T> NetObjManager* NetObjPtr<T>::get_obj_man() const {
        return objMan;
    }

    template <typename T> NetObjID NetObjPtr<T>::get_net_id() const {
        return id;
    }

    template <typename T> T* NetObjPtr<T>::get() const {
        return sharedPtr.get();
    }

    template <typename T> std::add_lvalue_reference_t<T> NetObjPtr<T>::operator*() const requires(!std::is_void_v<T>) {
        return *sharedPtr;
    }

    template <typename T> T* NetObjPtr<T>::operator->() const {
        return sharedPtr.get();
    }

    template <typename T> NetObjPtr<T>::operator bool() const {
        return sharedPtr != nullptr;
    }

    template <typename T> std::strong_ordering NetObjPtr<T>::operator<=>(const NetObjPtr<T>& rhs) const {
        return sharedPtr <=> rhs.sharedPtr;
    }

    template <typename T> void NetObjPtr<T>::send_client_update(const std::string& channel, std::function<void(const NetObjPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const {
        if(objMan->client && !objMan->client->is_disconnected()) {
            auto ss(std::make_shared<std::stringstream>());
            {
                cereal::PortableBinaryOutputArchive m(*ss);
                m(objMan->updateCommandID, id);
                sendUpdateFunc(*this, m);
            }
            objMan->client->send_string_stream_to_server(channel, ss);
        }
    }

    template <typename T> void NetObjPtr<T>::send_server_update_to_all_clients(const std::string& channel, std::function<void(const NetObjPtr<T>&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const {
        if(objMan->server && !objMan->server->is_disconnected()) {
            auto ss(std::make_shared<std::stringstream>());
            {
                cereal::PortableBinaryOutputArchive m(*ss);
                m(objMan->updateCommandID, id);
                sendUpdateFunc(*this, m);
            }
            objMan->server->send_string_stream_to_all_clients(channel, ss);
        }
    }

    template <typename T> void NetObjPtr<T>::write_create_message(cereal::PortableBinaryOutputArchive& a) const {
        auto& typeIndexData = objMan->isServer ? objMan->typeList->typeIndexDataServer[std::type_index(typeid(T*))] : objMan->typeList->typeIndexDataClient[std::type_index(typeid(T*))];
        a(id, typeIndexData.netTypeID);
        typeIndexData.writeConstructorFunc(this->cast<void>(), a);
    }

    template <typename T> NetObjPtr<T>::NetObjPtr(NetObjManager* initObjMan, NetObjID initID, const std::shared_ptr<T>& initPtr):
        objMan(initObjMan),
        id(initID),
        sharedPtr(initPtr)
    {}
}
