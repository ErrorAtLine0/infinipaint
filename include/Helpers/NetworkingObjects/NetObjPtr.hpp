#pragma once
#include "NetObjIDGenerator.hpp"
#include <compare>
#include <string>
#include <functional>
#include <cereal/archives/portable_binary.hpp>
#include "NetObjManager.hpp"
#include "NetObjManagerTypeList.hpp"

namespace NetworkingObjects {
    template <typename T> class NetObjPtr {
        public:
            NetObjPtr():
                objMan(nullptr),
                sharedPtr(nullptr)
            {}
            NetObjPtr(const NetObjPtr& other):
                objMan(other.objMan),
                id(other.id),
                sharedPtr(other.sharedPtr)
            {}
            NetObjPtr& operator=(const NetObjPtr& other) {
                if(this != &other) {
                    if(sharedPtr && sharedPtr.unique())
                        objMan->objectData.erase(id);
                    objMan = other.objMan;
                    id = other.id;
                    sharedPtr = other.sharedPtr;
                }
                return *this;
            }
            ~NetObjPtr() {
                if(sharedPtr && sharedPtr.unique())
                    objMan->objectData.erase(id);
            }

            NetObjID get_net_id() const {
                return id;
            }
            T* get() const {
                return sharedPtr.get();
            }
            T& operator*() const {
                return *sharedPtr;
            }
            operator bool() const {
                return sharedPtr != nullptr;
            }
            std::strong_ordering operator<=>(const NetObjPtr<T>& rhs) const {
                return sharedPtr <=> rhs.sharedPtr;
            }
            void send_client_update(const std::string& channel, std::function<void(T&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const {
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
            void send_server_update_to_all_clients(const std::string& channel, std::function<void(T&, cereal::PortableBinaryOutputArchive&)> sendUpdateFunc) const {
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
            void write_create_message(cereal::PortableBinaryOutputArchive& a) const {
                auto& typeIndexData = objMan->typeList->typeIndexData[std::type_index(typeid(T))];
                a(id, typeIndexData.netTypeID);
                typeIndexData.writeConstructorFunc(static_cast<void*>(this), a);
            }
        private:
            friend class NetObjManager;

            NetObjPtr(NetObjManager* initObjMan, NetObjID initID, const std::shared_ptr<T>& initPtr):
                objMan(initObjMan),
                id(initID),
                sharedPtr(initPtr)
            {}

            NetObjManager* objMan;
            NetObjID id;
            std::shared_ptr<T> sharedPtr;
    };

}
