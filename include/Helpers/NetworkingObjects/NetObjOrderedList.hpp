#pragma once
#include "Helpers/Networking/NetLibrary.hpp"
#include "NetObjOwnerPtr.hpp"
#include "NetObjTemporaryPtr.hpp"
#include "NetObjWeakPtr.hpp"
#include "NetObjID.hpp"
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/unordered_set.hpp>
#include <cereal/types/vector.hpp>
#include "NetObjManagerTypeList.hpp"
#include <limits>
#include <list>

namespace NetworkingObjects {
    enum class ObjPtrOrderedListCommand_StoC : uint8_t {
        INSERT_LIST_CONSTRUCT,
        INSERT_LIST_REFERENCE,
        ERASE_ID_LIST
    };

    enum class ObjPtrOrderedListCommand_CtoS : uint8_t {
        INSERT_LIST,
        ERASE_ID_LIST
    };

    template <typename T> struct NetObjOrderedListObjectInfo {
        NetObjOrderedListObjectInfo(NetObjOwnerPtr<T> iObj, uint32_t iPos):
            obj(std::move(iObj)),
            pos(iPos)
        {}
        bool operator<(NetObjOrderedListObjectInfo& other) const {
            return pos < other.pos;
        }
        NetObjOrderedListObjectInfo() = delete;
        NetObjOwnerPtr<T> obj;
        uint32_t pos;
    };

    template <typename T> using NetObjOrderedListIterator = std::list<NetObjOrderedListObjectInfo<T>>::iterator;
    template <typename T> using NetObjOrderedListConstIterator = std::list<NetObjOrderedListObjectInfo<T>>::const_iterator;

    template <typename T> void set_position_obj_info_list(NetObjOrderedListIterator<T> start, NetObjOrderedListIterator<T> end) {
        if(start == end)
            return;
        uint32_t i = start->pos;
        for(; start != end; ++start) {
            start->pos = i;
            ++i;
        }
    }

    template <typename T> class NetObjOrderedList {
        public:
            // Don't use this function unless you're sure that class T isn't a base class
            template <typename ...Args> NetObjOrderedListIterator<T> emplace_back_direct(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, Args&&... items) {
                std::vector<std::pair<NetObjOrderedListIterator<T>, NetObjOwnerPtr<T>>> newObjOwner;
                newObjOwner.emplace_back(l->end(), std::move(l.get_obj_man()->template make_obj_direct<T>(items...)));
                auto toRet = l->insert_ordered_list(l, nullptr, newObjOwner);
                return toRet.empty() ? l->end() : toRet.back();
            }
            static NetObjOrderedListIterator<T> push_back_and_send_create(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, T* newObj) {
                std::vector<std::pair<NetObjOrderedListIterator<T>, NetObjOwnerPtr<T>>> newObjOwner;
                newObjOwner.emplace_back(l->end(), std::move(l.get_obj_man()->template make_obj_from_ptr<T>(newObj)));
                auto toRet = l->insert_ordered_list(l, nullptr, newObjOwner);
                return toRet.empty() ? l->end() : toRet.back();
            }
            static NetObjOrderedListIterator<T> insert_and_send_create(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const NetObjOrderedListIterator<T>& it, T* newObj) {
                std::vector<std::pair<NetObjOrderedListIterator<T>, NetObjOwnerPtr<T>>> newObjOwner;
                newObjOwner.emplace_back(it, std::move(l.get_obj_man()->template make_obj_from_ptr<T>(newObj)));
                auto toRet = l->insert_ordered_list(l, nullptr, newObjOwner);
                return toRet.empty() ? l->end() : toRet.back();
            }
            static std::vector<NetObjOrderedListIterator<T>> insert_ordered_list_and_send_create(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::vector<std::pair<NetObjOrderedListIterator<T>, T*>>& newObjs) {
                if(newObjs.empty())
                    return {};
                std::vector<std::pair<NetObjOrderedListIterator<T>, NetObjOwnerPtr<T>>> ownerPtrOrderedList;
                for(uint32_t i = 0; i < static_cast<uint32_t>(newObjs.size()); i++)
                    ownerPtrOrderedList.emplace_back(newObjs[i].first, std::move(l.get_obj_man()->template make_obj_from_ptr<T>(newObjs[i].second)));
                return l->insert_ordered_list(l, nullptr, ownerPtrOrderedList);
            }
            static std::vector<NetObjOrderedListIterator<T>> insert_ordered_list_and_send_create(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, std::vector<std::pair<NetObjOrderedListIterator<T>, NetObjOwnerPtr<T>>>& newObjPtrs) {
                if(newObjPtrs.empty())
                    return {};
                return l->insert_ordered_list(l, nullptr, newObjPtrs);
            }
            static void erase_list(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::vector<NetObjOrderedListIterator<T>>& iterators, std::vector<NetObjOwnerPtr<T>>* erasedObjects = nullptr) {
                l->erase_it_list(l, iterators, erasedObjects);
            }
            static void erase(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const NetObjOrderedListIterator<T>& it) {
                l->erase_it_list(l, {it}, nullptr);
            }
            virtual void reassign_netobj_ids_call() = 0;

            typedef std::function<void(const NetObjOrderedListIterator<T>& objInfo)> InsertCallback;
            typedef std::function<void(const NetObjOrderedListIterator<T>& objInfo)> EraseCallback;
            typedef std::function<void(const NetObjOrderedListIterator<T>& objInfo, uint32_t oldPos)> MoveCallback;

            typedef NetObjOrderedListObjectInfo<T> value_type;

            void set_insert_callback(const InsertCallback& func) {
                insertCallback = func;
            }
            void set_erase_callback(const EraseCallback& func) {
                eraseCallback = func;
            }
            void set_move_callback(const MoveCallback& func) {
                moveCallback = func;
            }

            virtual bool contains(const NetObjID& p) const = 0;
            virtual NetObjOrderedListIterator<T> begin() = 0;
            virtual NetObjOrderedListIterator<T> end() = 0;
            virtual NetObjOrderedListIterator<T> at(uint32_t index) = 0;
            virtual NetObjOrderedListIterator<T> get(const NetObjID& id) = 0;
            virtual NetObjOrderedListConstIterator<T> begin() const = 0;
            virtual NetObjOrderedListConstIterator<T> end() const = 0;
            virtual NetObjOrderedListConstIterator<T> at(uint32_t index) const = 0;
            virtual NetObjOrderedListConstIterator<T> get(const NetObjID& id) const = 0;
            virtual uint32_t size() const = 0;
            virtual bool empty() const = 0;
            virtual ~NetObjOrderedList() {}
        protected:
            virtual std::vector<NetObjOrderedListIterator<T>> insert_ordered_list(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::shared_ptr<NetServer::ClientData>& clientInserting, std::vector<std::pair<NetObjOrderedListIterator<T>, NetObjOwnerPtr<T>>>& newObj) = 0;
            virtual void erase_it_list(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::vector<NetObjOrderedListIterator<T>>& itList, std::vector<NetObjOwnerPtr<T>>* erasedObjects) = 0;
            virtual void write_constructor(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryOutputArchive& a) = 0;
            virtual void read_update(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) = 0;
            virtual void read_constructor(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) = 0;

            // Call after inserting
            void call_insert_callback(const NetObjOrderedListIterator<T>& objInfo) {
                if(insertCallback)
                    insertCallback(objInfo);
            }
            void call_insert_callback_list(const std::vector<NetObjOrderedListIterator<T>>& objInfos) {
                if(insertCallback) {
                    for(auto objInfo : objInfos)
                        insertCallback(objInfo);
                }
            }

            // Call before erasing
            void call_erase_callback(const NetObjOrderedListIterator<T>& objInfo) {
                if(eraseCallback)
                    eraseCallback(objInfo);
            }

            void call_move_callback(const NetObjOrderedListIterator<T>& objInfo, uint32_t oldPos) {
                if(moveCallback)
                    moveCallback(objInfo, oldPos);
            }

        private:
            InsertCallback insertCallback;
            EraseCallback eraseCallback;
            MoveCallback moveCallback;
            template <typename S> friend void register_ordered_list_class(NetObjManager& t);
            static void write_constructor_func(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryOutputArchive& a) {
                l->write_constructor(l, a);
            }
            static void read_constructor_func(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) {
                l->read_constructor(l, a, c);
            }
            static void read_update_func(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) {
                l->read_update(l, a, c);
            }
    };

    template <typename T> class NetObjOrderedListServer : public NetObjOrderedList<T> {
        public:
            NetObjOrderedListServer() {}
            virtual bool contains(const NetObjID& id) const override {
                return idToDataMap.contains(id);
            }
            virtual uint32_t size() const override {
                return data.size();
            }
            virtual NetObjOrderedListIterator<T> begin() override {
                return data.begin();
            }
            virtual NetObjOrderedListIterator<T> end() override {
                return data.end();
            }
            virtual NetObjOrderedListIterator<T> at(uint32_t index) override {
                return std::next(data.begin(), index);
            }
            virtual NetObjOrderedListIterator<T> get(const NetObjID& id) override {
                auto it = idToDataMap.find(id);
                if(it == idToDataMap.end())
                    return data.end();
                return it->second;
            }
            virtual NetObjOrderedListConstIterator<T> begin() const override {
                return data.begin();
            }
            virtual NetObjOrderedListConstIterator<T> end() const override {
                return data.end();
            }
            virtual NetObjOrderedListConstIterator<T> at(uint32_t index) const override {
                return std::next(data.begin(), index);
            }
            virtual NetObjOrderedListConstIterator<T> get(const NetObjID& id) const override {
                auto it = idToDataMap.find(id);
                if(it == idToDataMap.end())
                    return data.end();
                return it->second;
            }
            virtual bool empty() const override {
                return data.empty();
            }
            virtual void reassign_netobj_ids_call() override {
                idToDataMap.clear();
                for(auto it = data.begin(); it != data.end(); ++it) {
                    it->obj.reassign_ids();
                    idToDataMap[it->obj.get_net_id()] = it;
                }
            }
        protected:
            virtual std::vector<NetObjOrderedListIterator<T>> insert_ordered_list(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::shared_ptr<NetServer::ClientData>& clientInserting, std::vector<std::pair<NetObjOrderedListIterator<T>, NetObjOwnerPtr<T>>>& newObjs) override {
                if(newObjs.empty())
                    return {};

                std::vector<NetworkingObjects::NetObjID> idPositionList;
                for(auto& [itToInsertAt, newObj] : newObjs) {
                    if(itToInsertAt == data.begin())
                        idPositionList.emplace_back(NetworkingObjects::NetObjID{0, 0});
                    else
                        idPositionList.emplace_back(std::prev(itToInsertAt)->obj.get_net_id());
                }

                std::vector<NetObjOrderedListIterator<T>> toRet;
                for(auto& [itToInsertAt, newObj] : newObjs) {
                    auto newObjIt = data.emplace(itToInsertAt, std::move(newObj), itToInsertAt == data.end() ? data.size() : itToInsertAt->pos); // All position values will be wrong except the first one, which is all that matters
                    idToDataMap.emplace(newObjIt->obj.get_net_id(), newObjIt);
                    toRet.emplace_back(newObjIt);
                }

                set_position_obj_info_list<T>(toRet[0], data.end());
                l.send_server_update_to_all_clients_except(clientInserting, RELIABLE_COMMAND_CHANNEL, [&toRet, &idPositionList](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::INSERT_LIST_CONSTRUCT, static_cast<uint32_t>(toRet.size()));
                    for(auto [newObjIt, idPosition] : std::views::zip(toRet, idPositionList)) {
                        a(idPosition);
                        newObjIt->obj.write_create_message(a);
                    }
                });
                l.send_server_update_to_client(clientInserting, RELIABLE_COMMAND_CHANNEL, [&toRet, &idPositionList](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::INSERT_LIST_REFERENCE, static_cast<uint32_t>(toRet.size()));
                    for(auto [newObjIt, idPosition] : std::views::zip(toRet, idPositionList))
                        a(idPosition, newObjIt->obj.get_net_id());
                });
                this->call_insert_callback_list(toRet);
                return toRet;
            }
            virtual void erase_it_list(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::vector<NetObjOrderedListIterator<T>>& itList, std::vector<NetObjOwnerPtr<T>>* erasedObjects) {
                if(itList.empty())
                    return;

                std::vector<std::pair<uint32_t, NetObjOwnerPtr<T>>> erasedObjectsWithPos;

                NetObjOrderedListIterator<T> smallestPosIt = data.end();
                std::vector<NetObjID> idListToSend;
                for(auto& it : itList) {
                    if(smallestPosIt == data.end() || smallestPosIt->pos >= it->pos) {
                        smallestPosIt = std::next(it);
                        if(smallestPosIt != data.end())
                            smallestPosIt->pos = it->pos;
                    }
                    idListToSend.emplace_back(it->obj.get_net_id());
                    this->call_erase_callback(it);
                    idToDataMap.erase(it->obj.get_net_id());
                    if(erasedObjects)
                        erasedObjectsWithPos.emplace_back(it->pos, std::move(it->obj));
                    data.erase(it);
                }

                if(erasedObjects) {
                    std::sort(erasedObjectsWithPos.begin(), erasedObjectsWithPos.end(), [](auto& a, auto& b) {
                        return a.first < b.first;
                    });
                    for(auto& e : erasedObjectsWithPos)
                        erasedObjects->emplace_back(std::move(e.second));
                }

                l.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [&idListToSend](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::ERASE_ID_LIST, idListToSend);
                });

                set_position_obj_info_list<T>(smallestPosIt, data.end());
            }
            virtual void write_constructor(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryOutputArchive& a) override {
                a(static_cast<uint32_t>(data.size()));
                for(auto& c : data)
                    c.obj.write_create_message(a);
            }
            virtual void read_constructor(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) override {
                uint32_t constructedSize;
                a(constructedSize);
                data.clear();
                idToDataMap.clear();
                for(uint32_t i = 0; i < constructedSize; i++) {
                    auto it = data.emplace(data.end(), l.get_obj_man()->template read_create_message<T>(a, c), i);
                    idToDataMap.emplace(data.back().obj.get_net_id(), it);
                }
                // Dont send the object data to all clients in the constructor, as the function that called read_create_message is also the one that will decide where to send the data of the constructed object
                // Example: the INSERT_SINGLE command in read_update, if it was inserting another NetObjOrderedList, would first construct the list using read_create_message, then it will call insert() to send it to clients
            }
            virtual void read_update(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) override {
                ObjPtrOrderedListCommand_CtoS command;
                a(command);
                switch(command) {
                    case ObjPtrOrderedListCommand_CtoS::INSERT_LIST: {
                        std::vector<std::pair<NetObjOrderedListIterator<T>, NetObjOwnerPtr<T>>> newObjs;

                        uint32_t newObjsSize;
                        a(newObjsSize);
                        for(uint32_t i = 0; i < newObjsSize; i++) {
                            NetworkingObjects::NetObjID idBeforeElement;
                            a(idBeforeElement);
                            NetObjOrderedListIterator<T> itToInsertAt;
                            if(idBeforeElement == NetworkingObjects::NetObjID{0, 0})
                                itToInsertAt = data.begin();
                            else {
                                auto idToDataMapIt = idToDataMap.find(idBeforeElement);
                                if(idToDataMapIt == idToDataMap.end())
                                    itToInsertAt = data.end();
                                else
                                    itToInsertAt = std::next(idToDataMapIt->second);
                            }

                            newObjs.emplace_back(itToInsertAt, l.get_obj_man()->template read_create_message<T>(a, c));
                        }
                        insert_ordered_list(l, c, newObjs);
                        break;
                    }
                    case ObjPtrOrderedListCommand_CtoS::ERASE_ID_LIST: {
                        std::vector<NetObjID> idsToErase;
                        a(idsToErase);
                        std::vector<NetObjOrderedListIterator<T>> iteratorsToErase;
                        for(auto& id : idsToErase) {
                            auto it = idToDataMap.find(id);
                            if(it != idToDataMap.end())
                                iteratorsToErase.emplace_back(it->second);
                        }
                        erase_it_list(l, iteratorsToErase, nullptr);
                        break;
                    }
                }
            }
        private:
            std::list<NetObjOrderedListObjectInfo<T>> data;
            std::unordered_map<NetObjID, NetObjOrderedListIterator<T>> idToDataMap;
    };

    template <typename T> class NetObjOrderedListClient : public NetObjOrderedList<T> {
        public:
            NetObjOrderedListClient() {}
            virtual bool contains(const NetObjID& id) const override {
                return clientIdToDataMap.contains(id);
            }
            virtual uint32_t size() const override {
                return clientData.size();
            }
            virtual NetObjOrderedListIterator<T> begin() override {
                return clientData.begin();
            }
            virtual NetObjOrderedListIterator<T> end() override {
                return clientData.end();
            }
            virtual NetObjOrderedListIterator<T> at(uint32_t index) override {
                return std::next(clientData.begin(), index);
            }
            virtual NetObjOrderedListIterator<T> get(const NetObjID& id) override {
                auto it = clientIdToDataMap.find(id);
                if(it == clientIdToDataMap.end())
                    return clientData.end();
                return it->second;
            }
            virtual NetObjOrderedListConstIterator<T> begin() const override {
                return clientData.begin();
            }
            virtual NetObjOrderedListConstIterator<T> end() const override {
                return clientData.end();
            }
            virtual NetObjOrderedListConstIterator<T> at(uint32_t index) const override {
                return std::next(clientData.begin(), index);
            }
            virtual NetObjOrderedListConstIterator<T> get(const NetObjID& id) const override {
                auto it = clientIdToDataMap.find(id);
                if(it == clientIdToDataMap.end())
                    return clientData.end();
                return it->second;
            }
            virtual bool empty() const override {
                return clientData.empty();
            }
            virtual void reassign_netobj_ids_call() override {
                serverData.clear();
                serverIdToDataMap.clear();
                clientIdToDataMap.clear();
                for(auto it = clientData.begin(); it != clientData.end(); ++it) {
                    it->obj.reassign_ids();
                    clientIdToDataMap[it->obj.get_net_id()] = it;
                    auto serverIt = serverData.emplace(serverData.end(), it->obj.get_net_id());
                    serverIdToDataMap[it->obj.get_net_id()] = serverIt;
                }
            }
        protected:
            virtual std::vector<NetObjOrderedListIterator<T>> insert_ordered_list(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::shared_ptr<NetServer::ClientData>& clientInserting, std::vector<std::pair<NetObjOrderedListIterator<T>, NetObjOwnerPtr<T>>>& newObjs) override {
                if(newObjs.empty())
                    return {};

                std::vector<NetworkingObjects::NetObjID> idPositionList;
                for(auto& [itToInsertAt, newObj] : newObjs) {
                    if(itToInsertAt == clientData.begin())
                        idPositionList.emplace_back(NetworkingObjects::NetObjID{0, 0});
                    else
                        idPositionList.emplace_back(std::prev(itToInsertAt)->obj.get_net_id());
                }

                std::vector<NetObjOrderedListIterator<T>> toRet;
                for(auto& [itToInsertAt, newObj] : newObjs) {
                    auto newObjIt = clientData.emplace(itToInsertAt, std::move(newObj), itToInsertAt == clientData.end() ? clientData.size() : itToInsertAt->pos); // All position values will be wrong except the first one, which is all that matters
                    clientIdToDataMap.emplace(newObjIt->obj.get_net_id(), newObjIt);
                    toRet.emplace_back(newObjIt);
                }
                set_position_obj_info_list<T>(toRet[0], clientData.end());
                l.send_client_update(RELIABLE_COMMAND_CHANNEL, [&toRet, &idPositionList](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_CtoS::INSERT_LIST, static_cast<uint32_t>(toRet.size()));
                    for(auto [newObjIt, idPosition] : std::views::zip(toRet, idPositionList)) {
                        a(idPosition);
                        newObjIt->obj.write_create_message(a);
                    }
                });
                this->call_insert_callback_list(toRet);
                return toRet;
            }
            virtual void erase_it_list(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::vector<NetObjOrderedListIterator<T>>& itList, std::vector<NetObjOwnerPtr<T>>* erasedObjects) {
                if(itList.empty())
                    return;

                std::vector<std::pair<uint32_t, NetObjOwnerPtr<T>>> erasedObjectsWithPos;
                NetObjOrderedListIterator<T> smallestPosIt = clientData.end();
                std::vector<NetObjID> idListToSend;
                for(auto& it : itList) {
                    if(smallestPosIt == clientData.end() || smallestPosIt->pos >= it->pos) {
                        smallestPosIt = std::next(it);
                        if(smallestPosIt != clientData.end())
                            smallestPosIt->pos = it->pos;
                    }
                    idListToSend.emplace_back(it->obj.get_net_id());
                    this->call_erase_callback(it);
                    clientIdToDataMap.erase(it->obj.get_net_id());
                    if(erasedObjects)
                        erasedObjectsWithPos.emplace_back(it->pos, std::move(it->obj));
                    clientData.erase(it);
                }

                if(erasedObjects) {
                    std::sort(erasedObjectsWithPos.begin(), erasedObjectsWithPos.end(), [](auto& a, auto& b) {
                        return a.first < b.first;
                    });
                    for(auto& e : erasedObjectsWithPos)
                        erasedObjects->emplace_back(std::move(e.second));
                }

                l.send_client_update(RELIABLE_COMMAND_CHANNEL, [&idListToSend](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_CtoS::ERASE_ID_LIST, idListToSend);
                });

                set_position_obj_info_list<T>(smallestPosIt, clientData.end());
            }
            virtual void write_constructor(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryOutputArchive& a) override {
                a(static_cast<uint32_t>(clientData.size()));
                serverData.clear();
                serverIdToDataMap.clear();
                for(auto& c : clientData) {
                    c.obj.write_create_message(a);
                    auto serverIt = serverData.emplace(serverData.end(), c.obj.get_net_id()); // We are the ones who made this list, so fill up
                    serverIdToDataMap.emplace(c.obj.get_net_id(), serverIt);
                }
            }
            virtual void read_constructor(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) override {
                uint32_t constructedSize;
                a(constructedSize);
                clientData.clear();
                serverData.clear();
                clientIdToDataMap.clear();
                serverIdToDataMap.clear();
                for(uint32_t i = 0; i < constructedSize; i++) {
                    auto clientIt = clientData.emplace(clientData.end(), l.get_obj_man()->template read_create_message<T>(a, nullptr), i);
                    clientIdToDataMap.emplace(clientIt->obj.get_net_id(), clientIt);
                    auto serverIt = serverData.emplace(serverData.end(), clientIt->obj.get_net_id());
                    serverIdToDataMap.emplace(clientIt->obj.get_net_id(), serverIt);
                }
            }
            virtual void read_update(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) override {
                ObjPtrOrderedListCommand_StoC c;
                a(c);
                switch(c) {
                    case ObjPtrOrderedListCommand_StoC::INSERT_LIST_CONSTRUCT: {
                        uint32_t newObjsSize;
                        a(newObjsSize);

                        std::vector<NetworkingObjects::NetObjID> idsBeforeElements;
                        std::vector<std::pair<NetObjOrderedListIterator<T>, NetObjOwnerPtr<T>>> newObjs;

                        for(uint32_t i = 0; i < newObjsSize; i++) {
                            auto& idBeforeElement = idsBeforeElements.emplace_back();
                            a(idBeforeElement);
                            newObjs.emplace_back(get_client_iterator_from_server_previous_net_obj_id(idBeforeElement), l.get_obj_man()->template read_create_message<T>(a, nullptr));
                        }

                        for(uint32_t i = 0; i < newObjsSize; i++) {
                            auto serverInsertAtIt = get_server_iterator_from_server_previous_net_obj_id(idsBeforeElements[i]);
                            auto serverIt = serverData.emplace(serverInsertAtIt, newObjs[i].second.get_net_id());
                            serverIdToDataMap.emplace(newObjs[i].second.get_net_id(), serverIt);
                        }

                        auto firstInsertedIt = clientData.end();
                        for(uint32_t i = 0; i < newObjs.size(); i++) {
                            auto& [itToInsertAt, newObj] = newObjs[i];
                            auto newObjIt = clientData.emplace(itToInsertAt, std::move(newObj), itToInsertAt == clientData.end() ? clientData.size() : itToInsertAt->pos); // All position values will be wrong except the first one, which is all that matters
                            clientIdToDataMap.emplace(newObjIt->obj.get_net_id(), newObjIt);
                            if(i == 0)
                                firstInsertedIt = newObjIt;
                        }
                        set_position_obj_info_list<T>(firstInsertedIt, clientData.end());

                        for(auto& newObj : newObjs)
                            this->call_insert_callback(newObj.first);

                        break;
                    }
                    case ObjPtrOrderedListCommand_StoC::INSERT_LIST_REFERENCE: {
                        uint32_t newObjsSize;
                        a(newObjsSize);

                        struct PerElementData {
                            NetworkingObjects::NetObjID idBeforeElement;
                            NetworkingObjects::NetObjID refID;
                            bool foundInClient;
                            NetObjOrderedListIterator<T> clientObjectIt;
                            NetObjOrderedListIterator<T> clientInsertPosIt;
                        };
                        std::vector<PerElementData> receivedElements;

                        for(uint32_t i = 0; i < newObjsSize; i++) {
                            auto& elemData = receivedElements.emplace_back();
                            a(elemData.idBeforeElement, elemData.refID);
                        }

                        std::list<NetObjOrderedListObjectInfo<T>> referredToObjectData;
                        std::unordered_map<NetworkingObjects::NetObjID, uint32_t> referredToObjectOldPos;

                        auto startRefreshFrom = clientData.end();

                        for(auto& elemData : receivedElements) {
                            auto clientIdToDataMapIt = clientIdToDataMap.find(elemData.refID);
                            if(clientIdToDataMapIt != clientIdToDataMap.end()) {
                                elemData.clientObjectIt = clientIdToDataMapIt->second;
                                referredToObjectOldPos.emplace(elemData.clientObjectIt->obj.get_net_id(), elemData.clientObjectIt->pos);
                                if(startRefreshFrom == clientData.end() || startRefreshFrom == elemData.clientObjectIt) {
                                    startRefreshFrom = std::next(elemData.clientObjectIt);
                                    if(startRefreshFrom != clientData.end())
                                        startRefreshFrom->pos = elemData.clientObjectIt->pos;
                                }
                                referredToObjectData.splice(referredToObjectData.end(), clientData, elemData.clientObjectIt, std::next(elemData.clientObjectIt));
                                clientIdToDataMap.erase(clientIdToDataMapIt);
                                elemData.foundInClient = true;
                            }
                            else
                                elemData.foundInClient = false;
                        }

                        set_position_obj_info_list<T>(startRefreshFrom, clientData.end());

                        for(auto& elemData : receivedElements) {
                            if(elemData.foundInClient)
                                elemData.clientInsertPosIt = get_client_iterator_from_server_previous_net_obj_id(elemData.idBeforeElement);
                        }

                        for(auto& elemData : receivedElements) {
                            auto serverInsertAtIt = get_server_iterator_from_server_previous_net_obj_id(elemData.idBeforeElement);
                            auto serverIt = serverData.emplace(serverInsertAtIt, elemData.refID);
                            serverIdToDataMap.emplace(elemData.refID, serverIt);
                        }

                        auto firstInsertedIt = clientData.end();
                        uint32_t firstInsertedActualPos = 0;
                        for(auto& elemData : receivedElements) {
                            if(!elemData.foundInClient)
                                continue;
                            if(firstInsertedIt == clientData.end()) {
                                firstInsertedActualPos = elemData.clientInsertPosIt->pos;
                                firstInsertedIt = elemData.clientObjectIt;
                            }
                            clientData.splice(elemData.clientInsertPosIt, referredToObjectData, elemData.clientObjectIt, std::next(elemData.clientObjectIt));
                            clientIdToDataMap.emplace(elemData.clientObjectIt->obj.get_net_id(), elemData.clientObjectIt);
                        }

                        // Instead of set_position_obj_info_list<T>
                        if(firstInsertedIt != clientData.end()) {
                            auto& it = firstInsertedIt;
                            auto& actualPos = firstInsertedActualPos;
                            for(; it != clientData.end(); ++it) {
                                it->pos = actualPos;
                                auto referredToObjectOldPosIt = referredToObjectOldPos.find(it->obj.get_net_id());
                                if(referredToObjectOldPosIt != referredToObjectOldPos.end()) {
                                    uint32_t oldPos = referredToObjectOldPosIt->second;
                                    if(oldPos != it->pos)
                                        this->call_move_callback(it, oldPos);
                                }
                                actualPos++;
                            }
                        }
                        break;
                    }
                    case ObjPtrOrderedListCommand_StoC::ERASE_ID_LIST: {
                        std::vector<NetworkingObjects::NetObjID> idsToErase;
                        a(idsToErase);

                        for(auto& id : idsToErase) {
                            auto serverIDToDataMapIt = serverIdToDataMap.find(id);
                            serverData.erase(serverIDToDataMapIt->second);
                            serverIdToDataMap.erase(serverIDToDataMapIt);
                        }

                        NetObjOrderedListIterator<T> smallestPosIt = clientData.end();
                        for(auto& id : idsToErase) {
                            auto clientIt = clientIdToDataMap.find(id);
                            if(clientIt == clientIdToDataMap.end())
                                continue;
                            auto it = clientIt->second;
                            if(smallestPosIt == clientData.end() || smallestPosIt->pos >= it->pos) {
                                smallestPosIt = std::next(it);
                                if(smallestPosIt != clientData.end())
                                    smallestPosIt->pos = it->pos;
                            }
                            this->call_erase_callback(it);
                            clientIdToDataMap.erase(clientIt);
                            clientData.erase(it);
                        }

                        set_position_obj_info_list<T>(smallestPosIt, clientData.end());
                        break;
                    }
                }
            }
        private:
            NetworkingObjects::NetObjOrderedListIterator<T> get_client_iterator_from_server_previous_net_obj_id(NetworkingObjects::NetObjID previousObjId) {
                if(previousObjId == NetworkingObjects::NetObjID{0, 0})
                    return clientData.begin();
                else {
                    auto serverIt = serverIdToDataMap[previousObjId];
                    for(;;) {
                        NetworkingObjects::NetObjID netObjID = *serverIt;
                        auto clientIt = clientIdToDataMap.find(netObjID);
                        if(clientIt != clientIdToDataMap.end())
                            return std::next(clientIt->second);
                        if(serverIt == serverData.begin())
                            return clientData.begin();
                        --serverIt;
                    }
                }
                return clientData.begin();
            }

            NetworkingObjects::NetObjOrderedListIterator<T> get_prev_client_iterator_from_server_previous_net_obj_id(NetworkingObjects::NetObjID previousObjId) {
                if(previousObjId == NetworkingObjects::NetObjID{0, 0})
                    return clientData.begin();
                else {
                    auto serverIt = serverIdToDataMap[previousObjId];
                    for(;;) {
                        NetworkingObjects::NetObjID netObjID = *serverIt;
                        auto clientIt = clientIdToDataMap.find(netObjID);
                        if(clientIt != clientIdToDataMap.end())
                            return clientIt->second;
                        if(serverIt == serverData.begin())
                            return clientData.begin();
                        --serverIt;
                    }
                }
                return clientData.begin();
            }

            std::list<NetObjID>::iterator get_server_iterator_from_server_previous_net_obj_id(NetworkingObjects::NetObjID previousObjId) {
                if(previousObjId == NetworkingObjects::NetObjID{0, 0})
                    return serverData.begin();
                return std::next(serverIdToDataMap[previousObjId]);
            }

            std::list<NetObjID> serverData;
            std::unordered_map<NetObjID, std::list<NetObjID>::iterator> serverIdToDataMap;
            std::list<NetObjOrderedListObjectInfo<T>> clientData;
            std::unordered_map<NetObjID, NetObjOrderedListIterator<T>> clientIdToDataMap;
    };

    template <typename S> void register_ordered_list_class(NetObjManager& objMan) {
        objMan.register_class<NetObjOrderedList<S>, NetObjOrderedList<S>, NetObjOrderedListClient<S>, NetObjOrderedListServer<S>>({
            .writeConstructorFuncClient = NetObjOrderedList<S>::write_constructor_func,
            .readConstructorFuncClient = NetObjOrderedList<S>::read_constructor_func,
            .readUpdateFuncClient = NetObjOrderedList<S>::read_update_func,
            .writeConstructorFuncServer = NetObjOrderedList<S>::write_constructor_func,
            .readConstructorFuncServer = NetObjOrderedList<S>::read_constructor_func,
            .readUpdateFuncServer = NetObjOrderedList<S>::read_update_func,
        });
    }
}
