#pragma once
#include "Helpers/Networking/NetLibrary.hpp"
#include "NetObjWeakPtr.hpp"
#include "cereal/archives/portable_binary.hpp"
#include <chrono>

namespace NetworkingObjects {
    class DelayUpdateSerializedClassManager {
        public:
            template <typename T> void register_class(NetObjManager& objMan) {
                objMan.register_class<T, T, T, T>({
                    .writeConstructorFuncClient = [&](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryOutputArchive& a){write_constructor_func<T>(o, a);},
                    .readConstructorFuncClient =  [&](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&){read_constructor_func<T>(o, a);},
                    .readUpdateFuncClient =       [&](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&){client_read_update_func<T>(o, a);},
                    .writeConstructorFuncServer = [&](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryOutputArchive& a){write_constructor_func<T>(o, a);},
                    .readConstructorFuncServer =  [&](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&){read_constructor_func<T>(o, a);},
                    .readUpdateFuncServer =       [&](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&){server_read_update_func<T>(o, a);}
                });
            }
            template <typename T> void send_update_to_all(const NetworkingObjects::NetObjTemporaryPtr<T>& o, const std::function<void(T&)>& postAssignmentFunc, bool finalUpdate) {
                o.send_update_to_all(finalUpdate ? RELIABLE_COMMAND_CHANNEL : UNRELIABLE_COMMAND_CHANNEL, [&](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                    if(o.get_obj_man()->is_server())
                        server_write_func(o, a, postAssignmentFunc);
                    else
                        client_write_update_func(o, a, postAssignmentFunc, finalUpdate);
                });
            }
            void update();
        private:
            template <typename T> void write_constructor_func(const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                a(*o);
            }

            template <typename T> void client_write_update_func(const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryOutputArchive& a, const std::function<void(T&)>& postAssignmentFunc, bool finalUpdate) {
                a(finalUpdate);
                a(*o);
                auto it = updatingObjs.find(o.get_net_id());

                if(it == updatingObjs.end()) {
                    UpdatingObject toAdd;
                    toAdd.objToUpdateCurrentData = std::static_pointer_cast<void>(std::make_shared<T>(*o));
                    toAdd.doLastAssignment = std::make_shared<bool>(true);
                    toAdd.lastUpdateTimePoint = std::chrono::steady_clock::now();
                    toAdd.setDataAfterTimeout = [postAssignmentFunc, oWeak = NetObjWeakPtr<T>(o), currentData = toAdd.objToUpdateCurrentData, doLastAssignment = toAdd.doLastAssignment]() {
                        NetObjTemporaryPtr<T> oLock = oWeak.lock();
                        if(oLock && *doLastAssignment) {
                            *oLock = *std::static_pointer_cast<T>(currentData);
                            if(postAssignmentFunc)
                                postAssignmentFunc(*oLock);
                        }
                    };
                    updatingObjs.emplace(o.get_net_id(), toAdd);
                }
                else {
                    auto& [netID, updatingObjData] = *it;
                    updatingObjData.lastUpdateTimePoint = std::chrono::steady_clock::now();
                }
            }

            template <typename T> void read_constructor_func(const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryInputArchive& a) {
                a(*o);
            }

            template <typename T> void client_read_update_func(const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryInputArchive& a) {
                auto it = updatingObjs.find(o.get_net_id());
                if(it == updatingObjs.end())
                    a(*o);
                else {
                    auto& [netID, updatingObjData] = *it;
                    a(*std::static_pointer_cast<T>(updatingObjData.objToUpdateCurrentData));
                    *updatingObjData.doLastAssignment = true;
                }
            }

            template <typename T> void server_write_func(const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryOutputArchive& a, const std::function<void(T&)>& postAssignmentFunc) {
                a(*o);
                auto it = updatingObjs.find(o.get_net_id());

                if(it == updatingObjs.end()) {
                    UpdatingObject toAdd;
                    toAdd.objToUpdateCurrentData = std::static_pointer_cast<void>(std::make_shared<T>(*o));
                    toAdd.lastUpdateTimePoint = std::chrono::steady_clock::now();
                    toAdd.doLastAssignment = std::make_shared<bool>(false);
                    toAdd.setDataAfterTimeout = [postAssignmentFunc, oWeak = NetObjWeakPtr<T>(o), currentData = toAdd.objToUpdateCurrentData, doLastAssignment = toAdd.doLastAssignment]() {
                        NetObjTemporaryPtr<T> oLock = oWeak.lock();
                        if(oLock && *doLastAssignment) {
                            *oLock = *std::static_pointer_cast<T>(currentData);
                            if(postAssignmentFunc)
                                postAssignmentFunc(*oLock);
                            oLock.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
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
                    *updatingObjData.doLastAssignment = false;
                }
            }

            template <typename T> void server_read_update_func(const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryInputArchive& a) {
                auto it = updatingObjs.find(o.get_net_id());
                bool finalUpdate;
                if(it == updatingObjs.end()) {
                    a(finalUpdate, *o);
                    o.send_server_update_to_all_clients(finalUpdate ? RELIABLE_COMMAND_CHANNEL : UNRELIABLE_COMMAND_CHANNEL, [](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                        a(*o);
                    });
                }
                else {
                    auto& [netID, updatingObjData] = *it;
                    a(finalUpdate, *std::static_pointer_cast<T>(updatingObjData.objToUpdateCurrentData));
                    *updatingObjData.doLastAssignment = true;
                }
            }

            struct UpdatingObject {
                std::shared_ptr<void> objToUpdateCurrentData;
                std::shared_ptr<bool> doLastAssignment;
                std::chrono::steady_clock::time_point lastUpdateTimePoint;
                std::function<void()> setDataAfterTimeout;
            };
            std::unordered_map<NetObjID, UpdatingObject> updatingObjs;
    };
}
