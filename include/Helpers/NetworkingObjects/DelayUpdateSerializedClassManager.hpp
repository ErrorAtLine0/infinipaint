#pragma once
#include "Helpers/Networking/NetLibrary.hpp"
#include "NetObjWeakPtr.hpp"
#include "cereal/archives/portable_binary.hpp"
#include <chrono>

namespace NetworkingObjects {
    class DelayUpdateSerializedClassManager {
        public:
            template <typename T> struct CustomConstructors {
                std::function<void(const T& o, cereal::PortableBinaryOutputArchive& a)> writeConstructor = [](const T& o, cereal::PortableBinaryOutputArchive& a) {
                    a(o);
                };
                std::function<void(T& o, cereal::PortableBinaryInputArchive& a)> readConstructor = [](T& o, cereal::PortableBinaryInputArchive& a) {
                    a(o);
                };
                std::function<void(const T& o, cereal::PortableBinaryOutputArchive& a)> writeUpdate = [](const T& o, cereal::PortableBinaryOutputArchive& a) {
                    a(o);
                };
                std::function<void(T& o, cereal::PortableBinaryInputArchive& a)> readUpdate = [](T& o, cereal::PortableBinaryInputArchive& a) {
                    a(o);
                };
                std::function<std::shared_ptr<T>(const T& o)> allocateCopy = [](const T& o) {
                    return std::make_shared<T>();
                };
                std::function<void(T&, const T&)> assignmentFunc = [](T& o, const T& o2) {
                    constexpr bool hasDefaultAssignment = requires(T& t, const T& t2) {
                        t = t2;
                    };
                    if constexpr(hasDefaultAssignment)
                        o = o2;
                };
                std::function<void(T&)> postUpdateFunc = [](T& o) {};
            };
            template <typename T> void register_class(NetObjManager& objMan, const CustomConstructors<T>& t = CustomConstructors<T>()) {
                objMan.register_class<T, T, T, T>({
                    .writeConstructorFuncClient = [writeConstructor = t.writeConstructor](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {writeConstructor(*o, a);},
                    .readConstructorFuncClient =  [readConstructor = t.readConstructor](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) {readConstructor(*o, a);},
                    .readUpdateFuncClient =       [&, readUpdate = t.readUpdate](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&){client_read_update_func<T>(o, a, readUpdate);},
                    .writeConstructorFuncServer = [writeConstructor = t.writeConstructor](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {writeConstructor(*o, a);},
                    .readConstructorFuncServer =  [readConstructor = t.readConstructor](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) {readConstructor(*o, a);},
                    .readUpdateFuncServer =       [&, readUpdate = t.readUpdate](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&){server_read_update_func<T>(o, a, readUpdate);}
                });
                typeIndexFuncs[std::type_index(typeid(T))] = {
                    .writeUpdateFunc = [writeFunc = t.writeUpdate](const void* o, cereal::PortableBinaryOutputArchive& a) {
                        writeFunc(*static_cast<const T*>(o), a);
                    },
                    .assignmentFunc = [assignFunc = t.assignmentFunc](void* o, const void* o2) {
                        assignFunc(*static_cast<T*>(o), *static_cast<const T*>(o2));
                    },
                    .postUpdateFunc = [postUpdateFunc = t.postUpdateFunc](void* o) {
                        postUpdateFunc(*static_cast<T*>(o));
                    },
                    .allocateCopyFunc = [allocateCopyFunc = t.allocateCopy](const void* o) {
                        return std::static_pointer_cast<void>(allocateCopyFunc(*static_cast<const T*>(o)));
                    }
                };
            }
            template <typename T> void send_update_to_all(const NetworkingObjects::NetObjTemporaryPtr<T>& o, bool finalUpdate) {
                // TODO: When finalUpdate = false, maybe add a small delay between updates instead of sending them every frame?
                o.send_update_to_all(finalUpdate ? RELIABLE_COMMAND_CHANNEL : UNRELIABLE_COMMAND_CHANNEL, [&](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                    if(o.get_obj_man()->is_server())
                        server_write_func(o, a);
                    else
                        client_write_update_func(o, a, finalUpdate);
                });
            }
            void update();
        private:
            template <typename T> void client_write_update_func(const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryOutputArchive& a, bool finalUpdate) {
                a(finalUpdate);
                typeIndexFuncs[std::type_index(typeid(T))].writeUpdateFunc(static_cast<const void*>(o.get()), a);
                auto it = updatingObjs.find(o.get_net_id());

                if(it == updatingObjs.end()) {
                    UpdatingObject toAdd;
                    toAdd.objToUpdateCurrentData = typeIndexFuncs[std::type_index(typeid(T))].allocateCopyFunc(static_cast<const void*>(o.get()));
                    toAdd.doLastAssignment = std::make_shared<bool>(true);
                    toAdd.lastUpdateTimePoint = std::chrono::steady_clock::now();
                    toAdd.setDataAfterTimeout = [&typeIndexFuncs = typeIndexFuncs, oWeak = NetObjWeakPtr<T>(o), currentData = toAdd.objToUpdateCurrentData, doLastAssignment = toAdd.doLastAssignment]() {
                        NetObjTemporaryPtr<T> oLock = oWeak.lock();
                        if(oLock && *doLastAssignment) {
                            typeIndexFuncs[std::type_index(typeid(T))].assignmentFunc(static_cast<void*>(oLock.get()), static_cast<const void*>(currentData.get()));
                            typeIndexFuncs[std::type_index(typeid(T))].postUpdateFunc(static_cast<void*>(oLock.get()));
                        }
                    };
                    updatingObjs.emplace(o.get_net_id(), toAdd);
                }
                else {
                    auto& [netID, updatingObjData] = *it;
                    updatingObjData.lastUpdateTimePoint = std::chrono::steady_clock::now();
                }
            }

            template <typename T> void client_read_update_func(const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::function<void(T&, cereal::PortableBinaryInputArchive&)>& readUpdate) {
                auto it = updatingObjs.find(o.get_net_id());
                if(it == updatingObjs.end()) {
                    readUpdate(*o, a);
                    typeIndexFuncs[std::type_index(typeid(T))].postUpdateFunc(static_cast<void*>(o.get()));
                }
                else {
                    auto& [netID, updatingObjData] = *it;
                    readUpdate(*std::static_pointer_cast<T>(updatingObjData.objToUpdateCurrentData), a);
                    *updatingObjData.doLastAssignment = true;
                }
            }

            template <typename T> void server_write_func(const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                typeIndexFuncs[std::type_index(typeid(T))].writeUpdateFunc(static_cast<const void*>(o.get()), a);
                auto it = updatingObjs.find(o.get_net_id());

                if(it == updatingObjs.end()) {
                    UpdatingObject toAdd;
                    toAdd.objToUpdateCurrentData = typeIndexFuncs[std::type_index(typeid(T))].allocateCopyFunc(static_cast<const void*>(o.get()));
                    toAdd.lastUpdateTimePoint = std::chrono::steady_clock::now();
                    toAdd.doLastAssignment = std::make_shared<bool>(false);
                    toAdd.setDataAfterTimeout = [&typeIndexFuncs = typeIndexFuncs, oWeak = NetObjWeakPtr<T>(o), currentData = toAdd.objToUpdateCurrentData, doLastAssignment = toAdd.doLastAssignment]() {
                        NetObjTemporaryPtr<T> oLock = oWeak.lock();
                        if(oLock && *doLastAssignment) {
                            typeIndexFuncs[std::type_index(typeid(T))].assignmentFunc(static_cast<void*>(oLock.get()), static_cast<const void*>(currentData.get()));
                            typeIndexFuncs[std::type_index(typeid(T))].postUpdateFunc(static_cast<void*>(oLock.get()));
                            oLock.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [&typeIndexFuncs = typeIndexFuncs](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                                typeIndexFuncs[std::type_index(typeid(T))].writeUpdateFunc(static_cast<const void*>(o.get()), a);
                            });
                        }
                    };
                    updatingObjs.emplace(o.get_net_id(), toAdd);
                }
                else {
                    auto& [netID, updatingObjData] = *it;
                    updatingObjData.lastUpdateTimePoint = std::chrono::steady_clock::now();
                    typeIndexFuncs[std::type_index(typeid(T))].assignmentFunc(static_cast<void*>(updatingObjs[o.get_net_id()].objToUpdateCurrentData.get()), static_cast<const void*>(o.get()));
                    *updatingObjData.doLastAssignment = false;
                }
            }

            template <typename T> void server_read_update_func(const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryInputArchive& a, const std::function<void(T&, cereal::PortableBinaryInputArchive&)>& readUpdate) {
                auto it = updatingObjs.find(o.get_net_id());
                bool finalUpdate;
                if(it == updatingObjs.end()) {
                    a(finalUpdate);
                    readUpdate(*o, a);
                    typeIndexFuncs[std::type_index(typeid(T))].postUpdateFunc(static_cast<void*>(o.get()));
                    o.send_server_update_to_all_clients(finalUpdate ? RELIABLE_COMMAND_CHANNEL : UNRELIABLE_COMMAND_CHANNEL, [&typeIndexFuncs = typeIndexFuncs](const NetObjTemporaryPtr<T>& o, cereal::PortableBinaryOutputArchive& a) {
                        typeIndexFuncs[std::type_index(typeid(T))].writeUpdateFunc(static_cast<const void*>(o.get()), a);
                    });
                }
                else {
                    auto& [netID, updatingObjData] = *it;
                    a(finalUpdate);
                    readUpdate(*std::static_pointer_cast<T>(updatingObjData.objToUpdateCurrentData), a);
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
            
            struct ExtraFunctions {
                std::function<void(const void*, cereal::PortableBinaryOutputArchive&)> writeUpdateFunc;
                std::function<void(void*, const void*)> assignmentFunc;
                std::function<void(void*)> postUpdateFunc;
                std::function<std::shared_ptr<void>(const void*)> allocateCopyFunc;
            };
            std::unordered_map<std::type_index, ExtraFunctions> typeIndexFuncs;
    };
}
