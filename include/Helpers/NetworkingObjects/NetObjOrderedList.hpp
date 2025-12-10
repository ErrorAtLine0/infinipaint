#pragma once
#include "Helpers/Networking/NetLibrary.hpp"
#include "NetObjPtr.hpp"
#include "NetObjID.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "NetObjManagerTypeList.hpp"

namespace NetworkingObjects {
    enum class ObjPtrOrderedListCommand_StoC : uint8_t {
        INSERT_SINGLE,
        ERASE_SINGLE
    };

    enum class ObjPtrOrderedListCommand_CtoS : uint8_t {
        INSERT_SINGLE,
        ERASE_SINGLE
    };

    template <typename T> struct NetObjOrderedListObjectInfo {
        NetObjOrderedListObjectInfo(const NetObjPtr<T>& iObj, uint32_t iPos, bool iSyncIgnore):
            obj(iObj),
            pos(iPos),
            syncIgnore(iSyncIgnore)
        {}
        NetObjOrderedListObjectInfo() = delete;
        NetObjPtr<T> obj;
        uint32_t pos;
        bool syncIgnore;
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
            template <typename ...Args> NetObjPtr<T> emplace_back_direct(const NetObjPtr<NetObjOrderedList<T>>& l, Args&&... items) {
                NetObjPtr<T> newObj = l.get_obj_man()->template make_obj_direct<T>(items...);
                l->insert(l, l->size(), newObj);
                return newObj;
            }
            static void push_back_and_send_create(const NetObjPtr<NetObjOrderedList<T>>& l, const NetObjPtr<T>& newObj) {
                l->insert(l, l->size(), newObj);
            }
            static void insert_and_send_create(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t posToInsertAt, const NetObjPtr<T>& newObj) {
                l->insert(l, posToInsertAt, newObj);
            }
            static NetObjPtr<T> erase(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t indexToErase) {
                return l->erase_by_index(l, indexToErase);
            }
            static uint32_t erase(const NetObjPtr<NetObjOrderedList<T>>& l, const NetObjPtr<T>& p) {
                return l->erase_by_ptr(l, p);
            }
            virtual uint32_t size() const = 0;
            virtual const NetObjPtr<T>& at(uint32_t index) const = 0;
            virtual bool empty() const = 0;
            virtual ~NetObjOrderedList() {}
        protected:
            virtual void insert(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t posToInsertAt, const NetObjPtr<T>& newObj) = 0;
            virtual NetObjPtr<T> erase_by_index(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t indexToErase) = 0;
            virtual uint32_t erase_by_ptr(const NetObjPtr<NetObjOrderedList<T>>& l, const NetObjPtr<T>& p) = 0;
            virtual void write_constructor(const NetObjPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryOutputArchive& a) = 0;
            virtual void read_update(const NetObjPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) = 0;
            virtual void read_constructor(const NetObjPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) = 0;
        private:
            template <typename S> friend void register_ordered_list_class(NetObjManagerTypeList& t);
            static void write_constructor_func(const NetObjPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryOutputArchive& a) {
                l->write_constructor(l, a);
            }
            static void read_constructor_func(const NetObjPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) {
                l->read_constructor(l, a, c);
            }
            static void read_update_func(const NetObjPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) {
                l->read_update(l, a, c);
            }
    };

    template <typename T> class NetObjOrderedListServer : public NetObjOrderedList<T> {
        public:
            NetObjOrderedListServer() {}
            virtual uint32_t size() const override {
                return data.size();
            }
            virtual const NetObjPtr<T>& at(uint32_t index) const override {
                return data[index]->obj;
            }
            virtual bool empty() const override {
                return data.empty();
            }
        protected:
            virtual void insert(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t posToInsertAt, const NetObjPtr<T>& newObj) override {
                uint32_t actualPosToInsertAt = posToInsertAt = std::min<uint32_t>(posToInsertAt, data.size());

                // Find duplicate. If found, erase the duplicate before inserting
                std::shared_ptr<NetObjOrderedListObjectInfo<T>> newObjInfoPtr;
                auto itObjAlreadyInData = idToDataMap.find(newObj.get_net_id());
                if(itObjAlreadyInData != idToDataMap.end()) {
                    uint32_t posToErase = itObjAlreadyInData->second->pos;
                    newObjInfoPtr = data[posToErase];
                    data.erase(data.begin() + posToErase);
                    if(posToErase < actualPosToInsertAt) // Shift insert position if an element was erased before the insert position
                        actualPosToInsertAt--;
                    newObjInfoPtr->pos = actualPosToInsertAt;
                    data.insert(data.begin() + actualPosToInsertAt, newObjInfoPtr);
                }
                else {
                    // Insert the new object if no duplicate found
                    newObjInfoPtr = std::make_shared<NetObjOrderedListObjectInfo<T>>(newObj, 0, false);
                    data.insert(data.begin() + actualPosToInsertAt, newObjInfoPtr);
                    idToDataMap.emplace(newObjInfoPtr->obj.get_net_id(), newObjInfoPtr);
                }

                l.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [&newObjInfoPtr, posToInsertAt](const NetObjPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::INSERT_SINGLE, posToInsertAt); // Dont send actual pos to insert at, since client will check for duplicates on its own and shift the index if necessary
                    newObjInfoPtr->obj.write_create_message(a);
                });
                set_positions_for_object_info_vector<T>(data, actualPosToInsertAt);
            }
            virtual NetObjPtr<T> erase_by_index(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t indexToErase) override {
                if(data.empty())
                    return {};
                indexToErase = std::min<uint32_t>(indexToErase, data.size() - 1);
                l.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [indexToErase](const NetObjPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::ERASE_SINGLE, indexToErase);
                });
                NetObjID idToErase = data[indexToErase]->obj.get_net_id();
                NetObjPtr<T> toRet = data[indexToErase]->obj;
                data.erase(data.begin() + indexToErase);
                idToDataMap.erase(idToErase);
                set_positions_for_object_info_vector<T>(data, indexToErase);
                return toRet;
            }
            virtual uint32_t erase_by_ptr(const NetObjPtr<NetObjOrderedList<T>>& l, const NetObjPtr<T>& p) override {
                if(data.empty())
                    return std::numeric_limits<uint32_t>::max();
                auto it = idToDataMap.find(p.get_net_id());
                if(it == idToDataMap.end())
                    return std::numeric_limits<uint32_t>::max();
                uint32_t indexToErase = it->second->pos;
                data.erase(data.begin() + indexToErase);
                idToDataMap.erase(it);
                l.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [indexToErase](const NetObjPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::ERASE_SINGLE, indexToErase);
                });
                set_positions_for_object_info_vector<T>(data, indexToErase);
                return indexToErase;
            }
            virtual void write_constructor(const NetObjPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryOutputArchive& a) override {
                a(static_cast<uint32_t>(data.size()));
                for(size_t i = 0; i < data.size(); i++)
                    data[i]->obj.write_create_message(a);
            }
            virtual void read_constructor(const NetObjPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) override {
                uint32_t constructedSize;
                a(constructedSize);
                data.clear();
                for(uint32_t i = 0; i < constructedSize; i++)
                    data.emplace_back(std::make_shared<NetObjOrderedListObjectInfo<T>>(l.get_obj_man()->template read_create_message<T>(a, c), static_cast<uint32_t>(data.size()), false));
                // Dont send the object data to all clients in the constructor, as the function that called read_create_message is also the one that will decide where to send the data of the constructed object
                // Example: the INSERT_SINGLE command in read_update, if it was inserting another NetObjOrderedList, would first construct the list using read_create_message, then it will call insert() to send it to clients
            }
            virtual void read_update(const NetObjPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) override {
                ObjPtrOrderedListCommand_CtoS command;
                a(command);
                switch(command) {
                    case ObjPtrOrderedListCommand_CtoS::INSERT_SINGLE:
                        uint32_t newPos;
                        a(newPos);
                        insert(l, newPos, l.get_obj_man()->template read_create_message<T>(a, c));
                        break;
                    case ObjPtrOrderedListCommand_CtoS::ERASE_SINGLE:
                        NetObjID idToErase;
                        a(idToErase);
                        auto it = idToDataMap.find(idToErase);
                        if(it != idToDataMap.end())
                            erase_by_index(l, it->second->pos);
                        break;
                }
            }
        private:
            std::vector<NetObjOrderedListObjectInfoPtr<T>> data;
            std::unordered_map<NetObjID, NetObjOrderedListObjectInfoPtr<T>> idToDataMap;
    };

    template <typename T> class NetObjOrderedListClient : public NetObjOrderedList<T> {
        public:
            NetObjOrderedListClient() {}
            virtual uint32_t size() const override {
                return clientData.size();
            }
            virtual const NetObjPtr<T>& at(uint32_t index) const override {
                return clientData[index]->obj;
            }
            virtual bool empty() const override {
                return clientData.empty();
            }
        protected:
            virtual void insert(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t posToInsertAt, const NetObjPtr<T>& newObj) {
                uint32_t actualPosToInsertAt = posToInsertAt = std::min<uint32_t>(posToInsertAt, clientData.size());

                NetObjOrderedListObjectInfoPtr<T> newObjInfoPtr;
                auto itObjAlreadyInData = clientIdToDataMap.find(newObj.get_net_id());
                if(itObjAlreadyInData != clientIdToDataMap.end()) {
                    uint32_t posToErase = itObjAlreadyInData->second->pos;
                    newObjInfoPtr = clientData[posToErase];
                    clientData.erase(clientData.begin() + posToErase);
                    if(posToErase < actualPosToInsertAt) // Shift insert position if an element was erased before the insert position
                        actualPosToInsertAt--;
                    clientData.insert(clientData.begin() + actualPosToInsertAt, newObjInfoPtr);
                }
                else {
                    auto serverIt = serverIdToDataMap.find(newObj.get_net_id()); // No duplicate in client data, but check if it's already in server data
                    if(serverIt != serverIdToDataMap.end())
                        newObjInfoPtr = serverIt->second;
                    else
                        newObjInfoPtr = std::make_shared<NetObjOrderedListObjectInfo<T>>(newObj, 0, true);
                    clientData.insert(clientData.begin() + actualPosToInsertAt, newObjInfoPtr);
                    clientIdToDataMap.emplace(newObjInfoPtr->obj.get_net_id(), newObjInfoPtr);
                }

                newObjInfoPtr->syncIgnore = true;

                l.send_client_update(RELIABLE_COMMAND_CHANNEL, [&newObjInfoPtr, posToInsertAt](const NetObjPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_CtoS::INSERT_SINGLE, posToInsertAt); // Don't put actual pos to insert at, since the server will check for duplicates and might shift the index on its own
                    newObjInfoPtr->obj.write_create_message(a);
                });
                set_positions_for_object_info_vector<T>(clientData, actualPosToInsertAt);
            }
            virtual NetObjPtr<T> erase_by_index(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t indexToErase) override {
                if(clientData.empty())
                    return {};
                indexToErase = std::min<uint32_t>(indexToErase, clientData.size() - 1);
                auto p = clientData[indexToErase];
                l.send_client_update(RELIABLE_COMMAND_CHANNEL, [&p](const NetObjPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_CtoS::ERASE_SINGLE, p->obj.get_net_id());
                });
                p->syncIgnore = true;
                uint32_t iToErase = p->pos;
                clientIdToDataMap.erase(p->obj.get_net_id());
                clientData.erase(clientData.begin() + iToErase);
                set_positions_for_object_info_vector<T>(clientData, iToErase);
                return p->obj;
            }
            virtual uint32_t erase_by_ptr(const NetObjPtr<NetObjOrderedList<T>>& l, const NetObjPtr<T>& p) override {
                if(clientData.empty())
                    return std::numeric_limits<uint32_t>::max();
                auto it = clientIdToDataMap.find(p.get_net_id());
                if(it == clientIdToDataMap.end())
                    return std::numeric_limits<uint32_t>::max();
                l.send_client_update(RELIABLE_COMMAND_CHANNEL, [&p](const NetObjPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_CtoS::ERASE_SINGLE, p.get_net_id());
                });
                it->second->syncIgnore = true;
                uint32_t iToErase = it->second->pos;
                auto toRet = clientData[iToErase];
                clientIdToDataMap.erase(it);
                clientData.erase(clientData.begin() + iToErase);
                set_positions_for_object_info_vector<T>(clientData, iToErase);
                return iToErase;
            }
            virtual void write_constructor(const NetObjPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryOutputArchive& a) override {
                a(static_cast<uint32_t>(clientData.size()));
                for(size_t i = 0; i < clientData.size(); i++)
                    clientData[i]->obj.write_create_message(a);
            }
            virtual void read_constructor(const NetObjPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) override {
                uint32_t constructedSize;
                a(constructedSize);
                clientData.clear();
                serverData.clear();
                for(uint32_t i = 0; i < constructedSize; i++)
                    serverData.emplace_back(std::make_shared<NetObjOrderedListObjectInfo<T>>(l.get_obj_man()->template read_create_message<T>(a, nullptr), static_cast<uint32_t>(serverData.size()), false));
                clientData = serverData;
            }
            virtual void read_update(const NetObjPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) override {
                ObjPtrOrderedListCommand_StoC c;
                a(c);
                switch(c) {
                    case ObjPtrOrderedListCommand_StoC::INSERT_SINGLE: {
                        uint32_t newServerPos;
                        a(newServerPos);
                        auto objToInsert = l.get_obj_man()->template read_create_message<T>(a, nullptr);
                        auto it = serverIdToDataMap.find(objToInsert.get_net_id());
                        if(it != serverIdToDataMap.end()) {
                            uint32_t serverPosErased = std::numeric_limits<uint32_t>::max();
                            std::erase_if(serverData, [&](auto& objInfo) {
                                if(objToInsert.get_net_id() == objInfo->obj.get_net_id()) {
                                    serverPosErased = &objInfo - &serverData[0];
                                    return true;
                                }
                                return false;
                            });
                            assert(serverPosErased != std::numeric_limits<uint32_t>::max());
                            if(serverPosErased < newServerPos)
                                newServerPos--; // Shift if there was a duplicate before this
                            //it->second->pos = newServerPos; // NOTE: DO NOT set the pos variable here, as pos is reserved for the position in the client vector
                            serverData.insert(serverData.begin() + newServerPos, it->second);
                        }
                        else {
                            NetObjOrderedListObjectInfoPtr<T> objInfoToInsert;
                            auto clientIt = clientIdToDataMap.find(objToInsert.get_net_id());
                            if(clientIt != clientIdToDataMap.end())
                                objInfoToInsert = clientIt->second;
                            else
                                objInfoToInsert = std::make_shared<NetObjOrderedListObjectInfo<T>>(objToInsert, 0, false);
                            objInfoToInsert->syncIgnore = false;
                            serverData.insert(serverData.begin() + newServerPos, objInfoToInsert);
                            serverIdToDataMap.emplace(objInfoToInsert->obj.get_net_id(), objInfoToInsert);
                        }
                        break;
                    }
                    case ObjPtrOrderedListCommand_StoC::ERASE_SINGLE: {
                        uint32_t serverPosToErase;
                        a(serverPosToErase);
                        NetObjID objID = serverData[serverPosToErase]->obj.get_net_id();
                        serverData.erase(serverData.begin() + serverPosToErase);
                        serverIdToDataMap.erase(objID);
                        break;
                    }
                }
                sync();
            }
        private:

            void sync() {
                std::vector<std::pair<uint32_t, NetObjOrderedListObjectInfoPtr<T>>> ignoredToReinsert;
                for(uint32_t i = 0; i < static_cast<uint32_t>(clientData.size()); i++)
                    if(clientData[i]->syncIgnore)
                        ignoredToReinsert.emplace_back(std::pair<uint32_t, NetObjOrderedListObjectInfoPtr<T>>{i, clientData[i]});
                clientData = serverData;
                clientIdToDataMap = serverIdToDataMap;
                std::erase_if(clientData, [&](const auto& obj) {
                    if(obj->syncIgnore) {
                        clientIdToDataMap.erase(obj->obj.get_net_id());
                        return true;
                    }
                    return false;
                });
                for(auto& p : ignoredToReinsert) {
                    if(p.first > clientData.size())
                        clientData.emplace_back(p.second);
                    else
                        clientData.insert(clientData.begin() + p.first, p.second);
                    clientIdToDataMap.emplace(p.second->obj.get_net_id(), p.second);
                }
                set_positions_for_object_info_vector<T>(clientData);
            }

            std::vector<NetObjOrderedListObjectInfoPtr<T>> serverData;
            std::unordered_map<NetObjID, NetObjOrderedListObjectInfoPtr<T>> serverIdToDataMap;

            std::vector<NetObjOrderedListObjectInfoPtr<T>> clientData;
            std::unordered_map<NetObjID, NetObjOrderedListObjectInfoPtr<T>> clientIdToDataMap;
    };

    template <typename S> void register_ordered_list_class(NetObjManagerTypeList& t) {
        t.register_class<NetObjOrderedList<S>, NetObjOrderedList<S>, NetObjOrderedListClient<S>, NetObjOrderedListServer<S>>({
            .writeConstructorFuncClient = NetObjOrderedList<S>::write_constructor_func,
            .readConstructorFuncClient = NetObjOrderedList<S>::read_constructor_func,
            .readUpdateFuncClient = NetObjOrderedList<S>::read_update_func,
            .writeConstructorFuncServer = NetObjOrderedList<S>::write_constructor_func,
            .readConstructorFuncServer = NetObjOrderedList<S>::read_constructor_func,
            .readUpdateFuncServer = NetObjOrderedList<S>::read_update_func,
        });
    }
}
