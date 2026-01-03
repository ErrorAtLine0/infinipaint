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
                return l->insert_single(l, nullptr, l->end(), l.get_obj_man()->template make_obj_direct<T>(items...));
            }
            static NetObjOrderedListIterator<T> push_back_and_send_create(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, T* newObj) {
                return l->insert_single(l, nullptr, l->end(), l.get_obj_man()->template make_obj_from_ptr<T>(newObj));
            }
            template <typename ...Args> NetObjOrderedListIterator<T> emplace_direct(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const NetObjOrderedListIterator<T>& it, Args&&... items) {
                return l->insert_single(l, nullptr, it, l.get_obj_man()->template make_obj_direct<T>(items...));
            }
            static NetObjOrderedListIterator<T> insert_and_send_create(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const NetObjOrderedListIterator<T>& it, T* newObj) {
                return l->insert_single(l, nullptr, it, l.get_obj_man()->template make_obj_from_ptr<T>(newObj));
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


            bool contains(const NetObjID& id) const {
                return data_map().contains(id);
            }
            uint32_t size() const {
                return data_list().size();
            }
            NetObjOrderedListIterator<T> begin() {
                return data_list().begin();
            }
            NetObjOrderedListIterator<T> end() {
                return data_list().end();
            }
            NetObjOrderedListIterator<T> at(uint32_t index) {
                auto& data = data_list();
                if(index >= data.size())
                    return data.end();
                return std::prev(data.end(), data.size() - index);
            }
            std::vector<NetObjOrderedListIterator<T>> at_ordered_indices(const std::vector<uint32_t>& orderedIndices) {
                auto& data = data_list();
                std::vector<NetObjOrderedListIterator<T>> toRet(orderedIndices.size());
                auto lastIt = data.end();
                uint32_t lastDataIndex = data.size();
                for(std::pair<const uint32_t&, NetObjOrderedListIterator<T>&> p : std::views::zip(orderedIndices, toRet) | std::views::reverse) {
                    uint32_t orderedIndex = std::min<uint32_t>(p.first, data.size());
                    NetObjOrderedListIterator<T>& it = p.second;
                    for(;;) {
                        if(orderedIndex == lastDataIndex) {
                            it = lastIt;
                            break;
                        }
                        --lastDataIndex;
                        --lastIt;
                    }
                }
                return toRet;
            }
            NetObjOrderedListIterator<T> get(const NetObjID& id) {
                auto it = data_map().find(id);
                if(it == data_map().end())
                    return data_list().end();
                return it->second;
            }
            std::vector<NetObjOrderedListIterator<T>> get_list(const std::vector<NetObjID>& ids) {
                std::vector<NetObjOrderedListIterator<T>> toRet;
                for(auto& id : ids)
                    toRet.emplace_back(get(id));
                return toRet;
            }
            NetObjOrderedListConstIterator<T> begin() const {
                return data_list().begin();
            }
            NetObjOrderedListConstIterator<T> end() const {
                return data_list().end();
            }
            NetObjOrderedListConstIterator<T> at(uint32_t index) const {
                auto& data = data_list();
                if(index >= data.size())
                    return data.end();
                return std::prev(data.begin(), data.size() - index);
            }
            std::vector<NetObjOrderedListConstIterator<T>> at_ordered_indices(const std::vector<uint32_t>& orderedIndices) const {
                auto& data = data_list();
                std::vector<NetObjOrderedListConstIterator<T>> toRet(orderedIndices.size());
                auto lastIt = data.end();
                uint32_t lastDataIndex = data.size();
                for(std::pair<const uint32_t&, NetObjOrderedListConstIterator<T>&> p : std::views::zip(orderedIndices, toRet)) {
                    uint32_t orderedIndex = std::min<uint32_t>(p.first, data.size());
                    NetObjOrderedListConstIterator<T>& it = p.second;
                    for(;;) {
                        if(lastDataIndex >= orderedIndex) {
                            it = lastIt;
                            break;
                        }
                        --lastDataIndex;
                        --lastIt;
                    }
                }
                return toRet;
            }
            NetObjOrderedListConstIterator<T> get(const NetObjID& id) const {
                auto it = data_map().find(id);
                if(it == data_map().end())
                    return data_list().end();
                return it->second;
            }
            std::vector<NetObjOrderedListConstIterator<T>> get_list(const std::vector<NetObjID>& ids) const {
                std::vector<NetObjOrderedListConstIterator<T>> toRet;
                for(auto& id : ids)
                    toRet.emplace_back(get(id));
                return toRet;
            }
            bool empty() const {
                return data_list().empty();
            }
            virtual ~NetObjOrderedList() {}
        protected:
            NetObjOrderedListIterator<T> insert_single(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::shared_ptr<NetServer::ClientData>& clientInserting, const NetObjOrderedListIterator<T> it, NetObjOwnerPtr<T> newObj) {
                std::vector<std::pair<NetObjOrderedListIterator<T>, NetObjOwnerPtr<T>>> toInsert;
                toInsert.emplace_back(it, std::move(newObj));
                return insert_ordered_list(l, clientInserting, toInsert).back();
            }
            virtual std::list<NetObjOrderedListObjectInfo<T>>& data_list() = 0;
            virtual const std::list<NetObjOrderedListObjectInfo<T>>& data_list() const = 0;
            virtual std::unordered_map<NetworkingObjects::NetObjID, NetObjOrderedListIterator<T>>& data_map() = 0;
            virtual const std::unordered_map<NetworkingObjects::NetObjID, NetObjOrderedListIterator<T>>& data_map() const = 0;
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
            virtual void reassign_netobj_ids_call() override {
                idToDataMap.clear();
                for(auto it = data.begin(); it != data.end(); ++it) {
                    it->obj.reassign_ids();
                    idToDataMap[it->obj.get_net_id()] = it;
                }
            }
        protected:
            virtual std::list<NetObjOrderedListObjectInfo<T>>& data_list() override { return data; }
            virtual const std::list<NetObjOrderedListObjectInfo<T>>& data_list() const override { return data; }
            virtual std::unordered_map<NetworkingObjects::NetObjID, NetObjOrderedListIterator<T>>& data_map() { return idToDataMap; }
            virtual const std::unordered_map<NetworkingObjects::NetObjID, NetObjOrderedListIterator<T>>& data_map() const { return idToDataMap; }
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
            }
            virtual void read_update(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) override {
                ObjPtrOrderedListCommand_CtoS command;
                a(command);
                switch(command) {
                    case ObjPtrOrderedListCommand_CtoS::INSERT_LIST: {
                        std::vector<std::pair<NetObjOrderedListIterator<T>, NetObjOwnerPtr<T>>> newObjs;
                        std::vector<uint32_t> insertIndices;

                        uint32_t newObjsSize;
                        a(newObjsSize);
                        for(uint32_t i = 0; i < newObjsSize; i++) {
                            uint32_t& insertIndex = insertIndices.emplace_back();
                            a(insertIndex);
                            insertIndex -= i;
                            insertIndex = std::min<uint32_t>(insertIndex, data.size());
                            newObjs.emplace_back(NetObjOrderedListIterator<T>(), l.get_obj_man()->template read_create_message<T>(a, c));
                        }

                        uint32_t currentIndex = data.size();
                        NetObjOrderedListIterator<T> currentIt = data.end();
                        for(std::pair<std::pair<NetObjOrderedListIterator<T>, NetObjOwnerPtr<T>>&, uint32_t&> elem : (std::views::zip(newObjs, insertIndices) | std::views::reverse)) {
                            uint32_t index = elem.second;
                            auto& it = elem.first.first;
                            while(currentIndex != index) {
                                --currentIndex;
                                --currentIt;
                            }
                            it = currentIt;
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
            virtual std::list<NetObjOrderedListObjectInfo<T>>& data_list() override { return clientData; }
            virtual const std::list<NetObjOrderedListObjectInfo<T>>& data_list() const override { return clientData; }
            virtual std::unordered_map<NetworkingObjects::NetObjID, NetObjOrderedListIterator<T>>& data_map() { return clientIdToDataMap; }
            virtual const std::unordered_map<NetworkingObjects::NetObjID, NetObjOrderedListIterator<T>>& data_map() const { return clientIdToDataMap; }
            virtual std::vector<NetObjOrderedListIterator<T>> insert_ordered_list(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::shared_ptr<NetServer::ClientData>& clientInserting, std::vector<std::pair<NetObjOrderedListIterator<T>, NetObjOwnerPtr<T>>>& newObjs) override {
                if(newObjs.empty())
                    return {};

                std::vector<NetObjOrderedListIterator<T>> toRet;
                for(auto& [itToInsertAt, newObj] : newObjs) {
                    auto newObjIt = clientData.emplace(itToInsertAt, std::move(newObj), itToInsertAt == clientData.end() ? clientData.size() : itToInsertAt->pos); // All position values will be wrong except the first one, which is all that matters
                    clientIdToDataMap.emplace(newObjIt->obj.get_net_id(), newObjIt);
                    toRet.emplace_back(newObjIt);
                }
                set_position_obj_info_list<T>(toRet[0], clientData.end());
                l.send_client_update(RELIABLE_COMMAND_CHANNEL, [&toRet](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_CtoS::INSERT_LIST, static_cast<uint32_t>(toRet.size()));
                    for(auto newObjIt : toRet) {
                        a(newObjIt->pos);
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

                        std::vector<NetObjOrderedListIterator<T>> insertedIterators;
                        auto firstInsertedIt = clientData.end();
                        for(uint32_t i = 0; i < newObjs.size(); i++) {
                            auto& [itToInsertAt, newObj] = newObjs[i];
                            auto newObjIt = clientData.emplace(itToInsertAt, std::move(newObj), itToInsertAt == clientData.end() ? clientData.size() : itToInsertAt->pos); // All position values will be wrong except the first one, which is all that matters
                            clientIdToDataMap.emplace(newObjIt->obj.get_net_id(), newObjIt);
                            insertedIterators.emplace_back(newObjIt);
                            if(i == 0)
                                firstInsertedIt = newObjIt;
                        }
                        set_position_obj_info_list<T>(firstInsertedIt, clientData.end());
                        this->call_insert_callback_list(insertedIterators);

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
