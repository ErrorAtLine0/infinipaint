#pragma once
#include "NetObjWeakPtr.hpp"
#include "cereal/archives/portable_binary.hpp"
#include <chrono>

namespace NetworkingObjects {
    class DelayUpdateSerializedClassManager {
        public:
            template <typename T> void register_class(NetObjManagerTypeList& t) {
                t.register_class<T, T, T, T>({
                    .writeConstructorFuncClient = [&](const NetObjPtr<T>& o, cereal::PortableBinaryOutputArchive& a){client_write_func<T>(o, a);},
                    .readConstructorFuncClient = [&](const NetObjPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&){delay_read_func<T>(o, a);},
                    .readUpdateFuncClient = [&](const NetObjPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&){delay_read_func<T>(o, a);},
                    .writeConstructorFuncServer = [&](const NetObjPtr<T>& o, cereal::PortableBinaryOutputArchive& a){server_write_func<T>(o, a);},
                    .readConstructorFuncServer = [&](const NetObjPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&){delay_read_func<T>(o, a);},
                    .readUpdateFuncServer = [&](const NetObjPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&){server_read_update_func<T>(o, a);}
                });
            }
            template <typename T> void send_update_to_all(const NetworkingObjects::NetObjPtr<T>& o) {
                o.send_update_to_all(RELIABLE_COMMAND_CHANNEL, [&](const NetObjPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                    if(o.get_obj_man()->is_server())
                        server_write_func(o, a);
                    else
                        client_write_func(o, a);
                });
            }
            void update();
        private:
            template <typename T> void client_write_func(const NetObjPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                a(*o);
                auto it = updatingObjs.find(o.get_net_id());
                if(it == updatingObjs.end()) {
                    UpdatingObject toAdd;
                    toAdd.objToUpdateCurrentData = std::static_pointer_cast<void>(std::make_shared<T>(*o));
                    toAdd.lastUpdateTimePoint = std::chrono::steady_clock::now();
                    toAdd.setDataAfterTimeout = [oWeak = NetObjWeakPtr<T>(o), currentData = toAdd.objToUpdateCurrentData]() {
                        NetObjPtr<T> oLock = oWeak.lock();
                        if(oLock)
                            *oLock = *std::static_pointer_cast<T>(currentData);
                    };
                    updatingObjs.emplace(o.get_net_id(), toAdd);
                }
                else {
                    auto& [netID, updatingObjData] = *it;
                    updatingObjData.lastUpdateTimePoint = std::chrono::steady_clock::now();
                }
            }

            template <typename T> void delay_read_func(const NetObjPtr<T>& o, cereal::PortableBinaryInputArchive& a) {
                auto it = updatingObjs.find(o.get_net_id());
                if(it == updatingObjs.end())
                    a(*o);
                else {
                    auto& [netID, updatingObjData] = *it;
                    a(*std::static_pointer_cast<T>(updatingObjData.objToUpdateCurrentData));
                }
            }

            template <typename T> void server_write_func(const NetObjPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                a(*o);
                auto it = updatingObjs.find(o.get_net_id());
                if(it == updatingObjs.end()) {
                    UpdatingObject toAdd;
                    toAdd.objToUpdateCurrentData = std::static_pointer_cast<void>(std::make_shared<T>(*o));
                    toAdd.lastUpdateTimePoint = std::chrono::steady_clock::now();
                    toAdd.setDataAfterTimeout = [oWeak = NetObjWeakPtr<T>(o), currentData = toAdd.objToUpdateCurrentData]() {
                        NetObjPtr<T> oLock = oWeak.lock();
                        if(oLock) {
                            *oLock = *std::static_pointer_cast<T>(currentData);
                            oLock.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [](const NetObjPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                                a(*o);
                            });
                        }
                    };
                    updatingObjs.emplace(o.get_net_id(), toAdd);
                }
                else {
                    auto& [netID, updatingObjData] = *it;
                    updatingObjData.lastUpdateTimePoint = std::chrono::steady_clock::now();
                    *std::static_pointer_cast<T>(updatingObjs[o.get_net_id()].objToUpdateCurrentData) = *o;
                }
            }

            template <typename T> void server_read_update_func(const NetObjPtr<T>& o, cereal::PortableBinaryInputArchive& a) {
                auto it = updatingObjs.find(o.get_net_id());
                if(it == updatingObjs.end()) {
                    a(*o);
                    o.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [](const NetObjPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                        a(*o);
                    });
                }
                else {
                    auto& [netID, updatingObjData] = *it;
                    a(*std::static_pointer_cast<T>(updatingObjData.objToUpdateCurrentData));
                }
            }

            struct UpdatingObject {
                std::shared_ptr<void> objToUpdateCurrentData;
                std::chrono::steady_clock::time_point lastUpdateTimePoint;
                std::function<void()> setDataAfterTimeout;
            };
            std::unordered_map<NetObjID, UpdatingObject> updatingObjs;
    };
}
