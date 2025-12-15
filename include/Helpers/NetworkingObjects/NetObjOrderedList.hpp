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

namespace NetworkingObjects {
    enum class ObjPtrOrderedListCommand_StoC : uint8_t {
        INSERT_SINGLE_CONSTRUCT,
        INSERT_SINGLE_REFERENCE,
        INSERT_MANY_CONSTRUCT,
        INSERT_MANY_REFERENCE,
        ERASE_SINGLE,
        ERASE_MANY
    };

    enum class ObjPtrOrderedListCommand_CtoS : uint8_t {
        INSERT_SINGLE,
        INSERT_MANY,
        ERASE_SINGLE,
        ERASE_MANY
    };

    template <typename T> struct NetObjOrderedListObjectInfo {
        NetObjOrderedListObjectInfo(NetObjOwnerPtr<T> iObj, uint32_t iPos):
            obj(std::move(iObj)),
            pos(iPos)
        {}
        NetObjOrderedListObjectInfo() = delete;
        NetObjOwnerPtr<T> obj;
        uint32_t pos;
    };

    template <typename T> using NetObjOrderedListObjectInfoPtr = std::shared_ptr<NetObjOrderedListObjectInfo<T>>;

    template <typename T> void set_positions_for_object_info_vector(std::vector<NetObjOrderedListObjectInfoPtr<T>>& v, uint32_t i = 0) {
        uint32_t vSize = static_cast<uint32_t>(v.size());
        for(; i < vSize; i++)
            v[i]->pos = i;
    }

    template <typename T> class NetObjOrderedList {
        public:
            // Don't use this function unless you're sure that class T isn't a base class
            template <typename ...Args> NetObjOrderedListObjectInfoPtr<T> emplace_back_direct(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, Args&&... items) {
                NetObjOwnerPtr<T> newObj = l.get_obj_man()->template make_obj_direct<T>(items...);
                return l->insert(l, nullptr, l->size(), std::move(newObj));
            }
            static NetObjOrderedListObjectInfoPtr<T> push_back_and_send_create(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, T* newObj) {
                NetObjOwnerPtr<T> newObjOwner = l.get_obj_man()->template make_obj_from_ptr<T>(newObj);
                return l->insert(l, nullptr, l->size(), std::move(newObjOwner));
            }
            static NetObjOrderedListObjectInfoPtr<T> insert_and_send_create(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, uint32_t posToInsertAt, T* newObj) {
                NetObjOwnerPtr<T> newObjOwner = l.get_obj_man()->template make_obj_from_ptr<T>(newObj);
                return l->insert(l, nullptr, posToInsertAt, std::move(newObjOwner));
            }
            static std::vector<NetObjOrderedListObjectInfoPtr<T>> insert_ordered_list_and_send_create(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::vector<std::pair<uint32_t, T*>>& newObjs) {
                if(newObjs.empty())
                    return {};
                std::vector<std::pair<uint32_t, NetObjOwnerPtr<T>>> ownerPtrOrderedList;
                for(uint32_t i = 0; i < static_cast<uint32_t>(newObjs.size()); i++)
                    ownerPtrOrderedList.emplace_back(newObjs[i].first, std::move(l.get_obj_man()->template make_obj_from_ptr<T>(newObjs[i].second)));
                return l->insert_ordered_list(l, nullptr, ownerPtrOrderedList);
            }
            static void erase_unordered_set(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::unordered_set<NetObjID>& ids) {
                l->erase_by_unordered_set_ids(l, ids);
            }
            static void erase(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, uint32_t indexToErase) {
                l->erase_by_index(l, indexToErase);
            }
            static uint32_t erase(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const NetObjID& id) {
                return l->erase_by_id(l, id);
            }

            typedef std::function<void(const NetObjOrderedListObjectInfoPtr<T>& objInfo)> InsertCallback;
            typedef std::function<void(const NetObjOrderedListObjectInfoPtr<T>& objInfo)> EraseCallback;
            typedef std::function<void(const NetObjOrderedListObjectInfoPtr<T>& objInfo, uint32_t oldPos)> MoveCallback;
            typedef std::function<void()> PostSyncCallback;

            void set_insert_callback(const InsertCallback& func) {
                insertCallback = func;
            }
            void set_erase_callback(const EraseCallback& func) {
                eraseCallback = func;
            }
            void set_move_callback(const MoveCallback& func) {
                moveCallback = func;
            }
            void set_post_sync_callback(const PostSyncCallback& func) {
                postSyncCallback = func;
            }

            virtual bool contains(const NetObjID& p) const = 0;
            virtual const std::vector<NetObjOrderedListObjectInfoPtr<T>>& get_data() const = 0;
            virtual uint32_t size() const = 0;
            virtual const NetObjOrderedListObjectInfoPtr<T>& at(uint32_t index) const = 0;
            virtual bool empty() const = 0;
            virtual ~NetObjOrderedList() {}
        protected:
            virtual NetObjOrderedListObjectInfoPtr<T> insert(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::shared_ptr<NetServer::ClientData>& clientInserting, uint32_t posToInsertAt, NetObjOwnerPtr<T> newObj) = 0;
            virtual std::vector<NetObjOrderedListObjectInfoPtr<T>> insert_ordered_list(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::shared_ptr<NetServer::ClientData>& clientInserting, std::vector<std::pair<uint32_t, NetObjOwnerPtr<T>>>& newObj) = 0;
            virtual void erase_by_index(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, uint32_t indexToErase) = 0;
            virtual uint32_t erase_by_id(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const NetObjID& id) = 0;
            virtual void erase_by_unordered_set_ids(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, std::unordered_set<NetObjID> ids) = 0;
            virtual void write_constructor(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryOutputArchive& a) = 0;
            virtual void read_update(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) = 0;
            virtual void read_constructor(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) = 0;

            // Call after inserting
            void call_insert_callback(const NetObjOrderedListObjectInfoPtr<T>& objInfo) {
                if(insertCallback)
                    insertCallback(objInfo);
            }
            void call_insert_callback_list(const std::vector<NetObjOrderedListObjectInfoPtr<T>>& objInfos) {
                if(insertCallback) {
                    for(auto objInfo : objInfos)
                        insertCallback(objInfo);
                }
            }

            // Call before erasing
            void call_erase_callback(const NetObjOrderedListObjectInfoPtr<T>& objInfo) {
                if(eraseCallback)
                    eraseCallback(objInfo);
            }

            void call_move_callback(const NetObjOrderedListObjectInfoPtr<T>& objInfo, uint32_t oldPos) {
                if(moveCallback)
                    moveCallback(objInfo, oldPos);
            }

            void call_post_sync_callback() {
                if(postSyncCallback)
                    postSyncCallback();
            }

        private:
            InsertCallback insertCallback;
            EraseCallback eraseCallback;
            MoveCallback moveCallback;
            PostSyncCallback postSyncCallback;
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
            virtual const std::vector<NetObjOrderedListObjectInfoPtr<T>>& get_data() const override {
                return data;
            }
            virtual uint32_t size() const override {
                return data.size();
            }
            virtual const NetObjOrderedListObjectInfoPtr<T>& at(uint32_t index) const override {
                return data[index];
            }
            virtual bool empty() const override {
                return data.empty();
            }
        protected:
            virtual NetObjOrderedListObjectInfoPtr<T> insert(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::shared_ptr<NetServer::ClientData>& clientInserting, uint32_t posToInsertAt, NetObjOwnerPtr<T> newObj) override {
                posToInsertAt = std::min<uint32_t>(posToInsertAt, data.size());
                auto newObjInfoPtr = std::make_shared<NetObjOrderedListObjectInfo<T>>(std::move(newObj), 0);
                data.insert(data.begin() + posToInsertAt, newObjInfoPtr);
                idToDataMap.emplace(newObjInfoPtr->obj.get_net_id(), newObjInfoPtr);
                l.send_server_update_to_all_clients_except(clientInserting, RELIABLE_COMMAND_CHANNEL, [&newObjInfoPtr, posToInsertAt](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::INSERT_SINGLE_CONSTRUCT, posToInsertAt);
                    newObjInfoPtr->obj.write_create_message(a);
                });
                l.send_server_update_to_client(clientInserting, RELIABLE_COMMAND_CHANNEL, [&newObjInfoPtr, posToInsertAt](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::INSERT_SINGLE_REFERENCE, posToInsertAt, newObjInfoPtr->obj.get_net_id());
                });
                set_positions_for_object_info_vector<T>(data, posToInsertAt);
                this->call_insert_callback(data[posToInsertAt]);
                return data[posToInsertAt];
            }
            virtual std::vector<NetObjOrderedListObjectInfoPtr<T>> insert_ordered_list(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::shared_ptr<NetServer::ClientData>& clientInserting, std::vector<std::pair<uint32_t, NetObjOwnerPtr<T>>>& newObjs) override {
                std::vector<NetObjOrderedListObjectInfoPtr<T>> toRet;
                for(auto& [pos, newObj] : newObjs) {
                    uint32_t posToInsertAt = std::min<uint32_t>(pos, data.size());
                    auto newObjInfoPtr = std::make_shared<NetObjOrderedListObjectInfo<T>>(std::move(newObj), 0);
                    data.insert(data.begin() + posToInsertAt, newObjInfoPtr);
                    idToDataMap.emplace(newObjInfoPtr->obj.get_net_id(), newObjInfoPtr);
                    toRet.emplace_back(data[posToInsertAt]);
                }
                l.send_server_update_to_all_clients_except(clientInserting, RELIABLE_COMMAND_CHANNEL, [&toRet](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::INSERT_MANY_CONSTRUCT, static_cast<uint32_t>(toRet.size()));
                    for(const NetObjOrderedListObjectInfoPtr<T>& objInfo : toRet) {
                        a(objInfo->pos);
                        objInfo->obj.write_create_message(a);
                    }
                });
                l.send_server_update_to_client(clientInserting, RELIABLE_COMMAND_CHANNEL, [&toRet](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::INSERT_MANY_REFERENCE, static_cast<uint32_t>(toRet.size()));
                    for(const NetObjOrderedListObjectInfoPtr<T>& objInfo : toRet)
                        a(objInfo->pos, objInfo->obj.get_net_id());
                });
                set_positions_for_object_info_vector<T>(data, newObjs[0].first);
                this->call_insert_callback_list(toRet);
                return toRet;
            }
            virtual void erase_by_index(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, uint32_t indexToErase) override {
                if(data.empty())
                    return;
                indexToErase = std::min<uint32_t>(indexToErase, data.size() - 1);
                l.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [indexToErase](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::ERASE_SINGLE, indexToErase);
                });
                NetObjID idToErase = data[indexToErase]->obj.get_net_id();
                this->call_erase_callback(data[indexToErase]);
                data.erase(data.begin() + indexToErase);
                idToDataMap.erase(idToErase);
                set_positions_for_object_info_vector<T>(data, indexToErase);
            }
            virtual uint32_t erase_by_id(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const NetObjID& id) override {
                if(data.empty())
                    return std::numeric_limits<uint32_t>::max();
                auto it = idToDataMap.find(id);
                if(it == idToDataMap.end())
                    return std::numeric_limits<uint32_t>::max();
                uint32_t indexToErase = it->second->pos;
                this->call_erase_callback(data[indexToErase]);
                data.erase(data.begin() + indexToErase);
                idToDataMap.erase(it);
                l.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [indexToErase](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::ERASE_SINGLE, indexToErase);
                });
                set_positions_for_object_info_vector<T>(data, indexToErase);
                return indexToErase;
            }
            virtual void erase_by_unordered_set_ids(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, std::unordered_set<NetObjID> ids) {
                std::vector<uint32_t> indicesToErase;
                std::erase_if(ids, [&](const NetObjID& id) {
                    auto it = idToDataMap.find(id);
                    if(it == idToDataMap.end())
                        return true;
                    indicesToErase.emplace_back(it->second->pos);
                    idToDataMap.erase(it); // Erase here since we already have the iterator
                    return false;
                });
                if(ids.empty())
                    return;
                std::sort(indicesToErase.begin(), indicesToErase.end());
                for(uint32_t indexToErase : indicesToErase | std::views::reverse) {
                    this->call_erase_callback(data[indexToErase]);
                    data.erase(data.begin() + indexToErase);
                }
                l.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [&indicesToErase](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::ERASE_MANY, indicesToErase);
                });
                set_positions_for_object_info_vector<T>(data, indicesToErase.front());
            }
            virtual void write_constructor(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryOutputArchive& a) override {
                a(static_cast<uint32_t>(data.size()));
                for(size_t i = 0; i < data.size(); i++)
                    data[i]->obj.write_create_message(a);
            }
            virtual void read_constructor(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) override {
                uint32_t constructedSize;
                a(constructedSize);
                data.clear();
                idToDataMap.clear();
                for(uint32_t i = 0; i < constructedSize; i++) {
                    data.emplace_back(std::make_shared<NetObjOrderedListObjectInfo<T>>(l.get_obj_man()->template read_create_message<T>(a, c), static_cast<uint32_t>(data.size())));
                    idToDataMap.emplace(data.back()->obj.get_net_id(), data.back());
                }
                // Dont send the object data to all clients in the constructor, as the function that called read_create_message is also the one that will decide where to send the data of the constructed object
                // Example: the INSERT_SINGLE command in read_update, if it was inserting another NetObjOrderedList, would first construct the list using read_create_message, then it will call insert() to send it to clients
            }
            virtual void read_update(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) override {
                ObjPtrOrderedListCommand_CtoS command;
                a(command);
                switch(command) {
                    case ObjPtrOrderedListCommand_CtoS::INSERT_SINGLE: {
                        uint32_t newPos;
                        a(newPos);
                        insert(l, c, newPos, l.get_obj_man()->template read_create_message<T>(a, c));
                        break;
                    }
                    case ObjPtrOrderedListCommand_CtoS::INSERT_MANY: {
                        std::vector<std::pair<uint32_t, NetObjOwnerPtr<T>>> newObjs;
                        uint32_t newObjsSize;
                        a(newObjsSize);
                        for(uint32_t i = 0; i < newObjsSize; i++) {
                            uint32_t pos;
                            a(pos);
                            newObjs.emplace_back(pos, l.get_obj_man()->template read_create_message<T>(a, c));
                        }
                        insert_ordered_list(l, c, newObjs);
                        break;
                    }
                    case ObjPtrOrderedListCommand_CtoS::ERASE_SINGLE: {
                        NetObjID idToErase;
                        a(idToErase);
                        auto it = idToDataMap.find(idToErase);
                        if(it != idToDataMap.end())
                            erase_by_index(l, it->second->pos);
                        break;
                    }
                    case ObjPtrOrderedListCommand_CtoS::ERASE_MANY: {
                        std::unordered_set<NetObjID> idsToErase;
                        a(idsToErase);
                        erase_by_unordered_set_ids(l, idsToErase);
                        break;
                    }
                }
            }
        private:
            std::vector<NetObjOrderedListObjectInfoPtr<T>> data;
            std::unordered_map<NetObjID, NetObjOrderedListObjectInfoPtr<T>> idToDataMap;
    };

    template <typename T> class NetObjOrderedListClient : public NetObjOrderedList<T> {
        public:
            NetObjOrderedListClient() {}
            virtual bool contains(const NetObjID& id) const override {
                return clientIdToDataMap.contains(id);
            }
            virtual const std::vector<NetObjOrderedListObjectInfoPtr<T>>& get_data() const override {
                return clientData;
            }
            virtual uint32_t size() const override {
                return clientData.size();
            }
            virtual const NetObjOrderedListObjectInfoPtr<T>& at(uint32_t index) const override {
                return clientData[index];
            }
            virtual bool empty() const override {
                return clientData.empty();
            }
        protected:
            virtual NetObjOrderedListObjectInfoPtr<T> insert(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::shared_ptr<NetServer::ClientData>&, uint32_t posToInsertAt, NetObjOwnerPtr<T> newObj) {
                posToInsertAt = std::min<uint32_t>(posToInsertAt, clientData.size());
                auto newObjInfoPtr = std::make_shared<NetObjOrderedListObjectInfo<T>>(std::move(newObj), 0);
                clientData.insert(clientData.begin() + posToInsertAt, newObjInfoPtr);
                clientIdToDataMap.emplace(newObjInfoPtr->obj.get_net_id(), newObjInfoPtr);
                clientJustInsertedSyncIgnore.emplace(newObjInfoPtr->obj.get_net_id(), newObjInfoPtr);
                l.send_client_update(RELIABLE_COMMAND_CHANNEL, [&newObjInfoPtr, posToInsertAt](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_CtoS::INSERT_SINGLE, posToInsertAt); // Don't put actual pos to insert at, since the server will check for duplicates and might shift the index on its own
                    newObjInfoPtr->obj.write_create_message(a);
                });
                set_positions_for_object_info_vector<T>(clientData, posToInsertAt);
                this->call_insert_callback(clientData[posToInsertAt]);
                return clientData[posToInsertAt];
            }
            virtual std::vector<NetObjOrderedListObjectInfoPtr<T>> insert_ordered_list(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::shared_ptr<NetServer::ClientData>& clientInserting, std::vector<std::pair<uint32_t, NetObjOwnerPtr<T>>>& newObjs) override {
                std::vector<NetObjOrderedListObjectInfoPtr<T>> toRet;
                for(auto& [pos, newObj] : newObjs) {
                    uint32_t posToInsertAt = std::min<uint32_t>(pos, clientData.size());
                    auto newObjInfoPtr = std::make_shared<NetObjOrderedListObjectInfo<T>>(std::move(newObj), 0);
                    clientData.insert(clientData.begin() + posToInsertAt, newObjInfoPtr);
                    clientIdToDataMap.emplace(newObjInfoPtr->obj.get_net_id(), newObjInfoPtr);
                    clientJustInsertedSyncIgnore.emplace(newObjInfoPtr->obj.get_net_id(), newObjInfoPtr);
                    toRet.emplace_back(clientData[posToInsertAt]);
                }
                l.send_client_update(RELIABLE_COMMAND_CHANNEL, [&toRet](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_CtoS::INSERT_MANY, static_cast<uint32_t>(toRet.size()));
                    for(const NetObjOrderedListObjectInfoPtr<T>& objInfo : toRet) {
                        a(objInfo->pos);
                        objInfo->obj.write_create_message(a);
                    }
                });
                set_positions_for_object_info_vector<T>(clientData, newObjs[0].first);
                this->call_insert_callback_list(toRet);
                return toRet;
            }
            virtual void erase_by_index(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, uint32_t indexToErase) override {
                if(clientData.empty())
                    return;
                indexToErase = std::min<uint32_t>(indexToErase, clientData.size() - 1);
                auto p = clientData[indexToErase];
                l.send_client_update(RELIABLE_COMMAND_CHANNEL, [&p](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_CtoS::ERASE_SINGLE, p->obj.get_net_id());
                });
                uint32_t iToErase = p->pos;
                this->call_erase_callback(clientData[indexToErase]);
                clientIdToDataMap.erase(p->obj.get_net_id());
                clientJustInsertedSyncIgnore.erase(p->obj.get_net_id());
                clientData.erase(clientData.begin() + iToErase);
                set_positions_for_object_info_vector<T>(clientData, iToErase);
            }
            virtual uint32_t erase_by_id(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const NetObjID& id) override {
                if(clientData.empty())
                    return std::numeric_limits<uint32_t>::max();
                auto it = clientIdToDataMap.find(id);
                if(it == clientIdToDataMap.end())
                    return std::numeric_limits<uint32_t>::max();
                l.send_client_update(RELIABLE_COMMAND_CHANNEL, [&id](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_CtoS::ERASE_SINGLE, id);
                });
                uint32_t iToErase = it->second->pos;
                auto toRet = clientData[iToErase];
                this->call_erase_callback(clientData[iToErase]);
                clientIdToDataMap.erase(it);
                clientJustInsertedSyncIgnore.erase(id);
                clientData.erase(clientData.begin() + iToErase);
                set_positions_for_object_info_vector<T>(clientData, iToErase);
                return iToErase;
            }
            virtual void erase_by_unordered_set_ids(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, std::unordered_set<NetObjID> ids) {
                std::vector<uint32_t> indicesToErase;
                std::erase_if(ids, [&](const NetObjID& id) {
                    auto it = clientIdToDataMap.find(id);
                    if(it == clientIdToDataMap.end())
                        return true;
                    indicesToErase.emplace_back(it->second->pos);
                    clientIdToDataMap.erase(it); // Erase here since we already have the iterator
                    clientJustInsertedSyncIgnore.erase(id);
                    return false;
                });
                if(ids.empty())
                    return;
                std::sort(indicesToErase.begin(), indicesToErase.end());
                for(uint32_t indexToErase : indicesToErase | std::views::reverse) {
                    this->call_erase_callback(clientData[indexToErase]);
                    clientData.erase(clientData.begin() + indexToErase);
                }
                l.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [&ids](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_CtoS::ERASE_MANY, ids);
                });
                set_positions_for_object_info_vector<T>(clientData, indicesToErase.front());
            }
            virtual void write_constructor(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryOutputArchive& a) override {
                a(static_cast<uint32_t>(clientData.size()));
                for(size_t i = 0; i < clientData.size(); i++)
                    clientData[i]->obj.write_create_message(a);
            }
            virtual void read_constructor(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) override {
                uint32_t constructedSize;
                a(constructedSize);
                clientData.clear();
                serverData.clear();
                clientIdToDataMap.clear();
                for(uint32_t i = 0; i < constructedSize; i++) {
                    clientData.emplace_back(std::make_shared<NetObjOrderedListObjectInfo<T>>(l.get_obj_man()->template read_create_message<T>(a, nullptr), static_cast<uint32_t>(serverData.size())));
                    clientIdToDataMap.emplace(clientData.back()->obj.get_net_id(), clientData.back());
                    serverData.emplace_back(clientData.back()->obj.get_net_id());
                }
            }
            virtual void read_update(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) override {
                ObjPtrOrderedListCommand_StoC c;
                a(c);
                switch(c) {
                    case ObjPtrOrderedListCommand_StoC::INSERT_SINGLE_CONSTRUCT: {
                        uint32_t newServerPos;
                        a(newServerPos);
                        auto objInfoToInsert = std::make_shared<NetObjOrderedListObjectInfo<T>>(l.get_obj_man()->template read_create_message<T>(a, nullptr), std::numeric_limits<uint32_t>::max());
                        serverData.insert(serverData.begin() + newServerPos, objInfoToInsert->obj.get_net_id());
                        clientJustInsertedSyncIgnore.erase(objInfoToInsert->obj.get_net_id());

                        std::unordered_map<NetObjID, NetObjOrderedListObjectInfoPtr<T>> serverNewObjects;
                        serverNewObjects.emplace(objInfoToInsert->obj.get_net_id(), objInfoToInsert);
                        sync(serverNewObjects);
                        break;
                    }
                    case ObjPtrOrderedListCommand_StoC::INSERT_SINGLE_REFERENCE: {
                        NetObjID newObjID;
                        uint32_t newServerPos;
                        a(newServerPos, newObjID);
                        serverData.insert(serverData.begin() + newServerPos, newObjID);
                        clientJustInsertedSyncIgnore.erase(newObjID);
                        sync(std::unordered_map<NetObjID, NetObjOrderedListObjectInfoPtr<T>>());
                        break;
                    }
                    case ObjPtrOrderedListCommand_StoC::INSERT_MANY_CONSTRUCT: {
                        std::unordered_map<NetObjID, NetObjOrderedListObjectInfoPtr<T>> serverNewObjects;
                        uint32_t newObjsSize;
                        a(newObjsSize);
                        for(uint32_t i = 0; i < newObjsSize; i++) {
                            uint32_t newServerPos;
                            a(newServerPos);
                            auto objInfoToInsert = std::make_shared<NetObjOrderedListObjectInfo<T>>(l.get_obj_man()->template read_create_message<T>(a, nullptr), std::numeric_limits<uint32_t>::max());
                            serverData.insert(serverData.begin() + newServerPos, objInfoToInsert->obj.get_net_id());
                            clientJustInsertedSyncIgnore.erase(objInfoToInsert->obj.get_net_id());
                            serverNewObjects.emplace(objInfoToInsert->obj.get_net_id(), objInfoToInsert);
                        }
                        sync(serverNewObjects);
                        break;
                    }
                    case ObjPtrOrderedListCommand_StoC::INSERT_MANY_REFERENCE: {
                        uint32_t newObjsSize;
                        a(newObjsSize);
                        for(uint32_t i = 0; i < newObjsSize; i++) {
                            NetObjID newObjID;
                            uint32_t newServerPos;
                            a(newServerPos, newObjID);
                            serverData.insert(serverData.begin() + newServerPos, newObjID);
                            clientJustInsertedSyncIgnore.erase(newObjID);
                        }
                        sync(std::unordered_map<NetObjID, NetObjOrderedListObjectInfoPtr<T>>());
                        break;
                    }
                    case ObjPtrOrderedListCommand_StoC::ERASE_SINGLE: {
                        uint32_t serverPosToErase;
                        a(serverPosToErase);
                        NetObjID objID = serverData[serverPosToErase];
                        serverData.erase(serverData.begin() + serverPosToErase);
                        auto it = clientIdToDataMap.find(objID);
                        if(it != clientIdToDataMap.end()) {
                            uint32_t clientPosToErase = it->second->pos;
                            this->call_erase_callback(clientData[clientPosToErase]);
                            clientIdToDataMap.erase(it);
                            clientJustInsertedSyncIgnore.erase(objID);
                            clientData.erase(clientData.begin() + clientPosToErase);
                            set_positions_for_object_info_vector<T>(clientData, clientPosToErase);
                        }
                        break;
                    }
                    case ObjPtrOrderedListCommand_StoC::ERASE_MANY: {
                        std::vector<uint32_t> serverPositionsToErase;
                        a(serverPositionsToErase);
                        std::vector<uint32_t> clientIndicesToErase;
                        for(uint32_t serverPosToErase : serverPositionsToErase | std::views::reverse) {
                            NetObjID objID = serverData[serverPosToErase];
                            serverData.erase(serverData.begin() + serverPosToErase);
                            auto it = clientIdToDataMap.find(objID);
                            if(it != clientIdToDataMap.end()) {
                                clientIdToDataMap.erase(it);
                                clientJustInsertedSyncIgnore.erase(objID);
                                clientIndicesToErase.emplace_back(it->second->pos);
                            }
                        }
                        std::sort(clientIndicesToErase.begin(), clientIndicesToErase.end());
                        if(clientIndicesToErase.empty())
                            break;
                        for(uint32_t indexToErase : clientIndicesToErase | std::views::reverse) {
                            this->call_erase_callback(clientData[indexToErase]);
                            clientData.erase(clientData.begin() + indexToErase);
                        }
                        set_positions_for_object_info_vector<T>(clientData, clientIndicesToErase.front());
                        break;
                    }
                }
            }
        private:
            void sync(const std::unordered_map<NetObjID, NetObjOrderedListObjectInfoPtr<T>>& newServerObjects) {
                std::map<uint32_t, NetObjOrderedListObjectInfoPtr<T>> ignoredToReinsert;
                for(auto& [netID, objInfoPtr] : clientJustInsertedSyncIgnore)
                    ignoredToReinsert.emplace(objInfoPtr->pos, objInfoPtr);

                clientData.clear();
                for(const NetObjID& id : serverData) {
                    auto itFoundInClient = clientIdToDataMap.find(id);
                    if(itFoundInClient != clientIdToDataMap.end())
                        clientData.emplace_back(itFoundInClient->second);
                    else {
                        auto itFoundInServer = newServerObjects.find(id);
                        if(itFoundInServer != newServerObjects.end()) {
                            clientData.emplace_back(itFoundInServer->second);
                            clientIdToDataMap.emplace(itFoundInServer->first, itFoundInServer->second);
                        }
                    }
                }

                for(auto& [pos, objInfoPtr] : ignoredToReinsert) {
                    if(pos > clientData.size())
                        clientData.emplace_back(objInfoPtr);
                    else
                        clientData.insert(clientData.begin() + pos, objInfoPtr);
                }

                // Code that tries to not call post_sync_callback, which refreshes the entire cache.
                //uint32_t bumpFromServerInsert = 0;
                //for(uint32_t i = 0; i < clientData.size(); i++) {
                //    if(clientData[i]->pos == std::numeric_limits<uint32_t>::max()) {
                //        clientData[i]->pos = i;
                //        this->call_insert_callback(clientData[i]);
                //        bumpFromServerInsert++;
                //    }
                //    else if(clientData[i]->pos != (i + bumpFromServerInsert)) {
                //        uint32_t oldPos = clientData[i]->pos;
                //        clientData[i]->pos = i;
                //        this->call_move_callback(clientData[i], oldPos);
                //    }
                //    clientData[i]->pos = i;
                //}

                // Code that calls post_sync_callback
                set_positions_for_object_info_vector<T>(clientData);
                for(auto& [id, objInfo] : newServerObjects)
                    this->call_insert_callback(objInfo);
                this->call_post_sync_callback();
            }

            std::vector<NetObjID> serverData;
            std::vector<NetObjOrderedListObjectInfoPtr<T>> clientData;
            std::unordered_map<NetObjID, NetObjOrderedListObjectInfoPtr<T>> clientIdToDataMap;
            std::unordered_map<NetObjID, NetObjOrderedListObjectInfoPtr<T>> clientJustInsertedSyncIgnore;
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
