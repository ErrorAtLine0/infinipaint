#pragma once
#include "Helpers/Networking/NetLibrary.hpp"
#include "NetObjOwnerPtr.hpp"
#include "NetObjTemporaryPtr.hpp"
#include "NetObjID.hpp"
#include "cereal/archives/portable_binary.hpp"
#include "NetObjManagerTypeList.hpp"

namespace NetworkingObjects {
    enum class ObjPtrOrderedListCommand_StoC : uint8_t {
        INSERT_SINGLE_CONSTRUCT,
        INSERT_SINGLE_REFERENCE,
        ERASE_SINGLE
    };

    enum class ObjPtrOrderedListCommand_CtoS : uint8_t {
        INSERT_SINGLE,
        ERASE_SINGLE
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
            template <typename ...Args> NetObjTemporaryPtr<T> emplace_back_direct(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, Args&&... items) {
                NetObjOwnerPtr<T> newObj = l.get_obj_man()->template make_obj_direct<T>(items...);
                NetObjTemporaryPtr<T> tempPtr(newObj);
                l->insert(l, nullptr, l->size(), std::move(newObj));
                return tempPtr;
            }
            static void push_back_and_send_create(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, T* newObj) {
                l->insert(l, nullptr, l->size(), newObj);
            }
            static void insert_and_send_create(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, uint32_t posToInsertAt, const NetObjTemporaryPtr<T>& newObj) {
                l->insert(l, nullptr, posToInsertAt, newObj);
            }
            static void erase(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, uint32_t indexToErase) {
                return l->erase_by_index(l, indexToErase);
            }
            static uint32_t erase(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const NetObjTemporaryPtr<T>& p) {
                return l->erase_by_ptr(l, p);
            }
            virtual bool contains(const NetObjTemporaryPtr<T>& p) const = 0;
            virtual uint32_t size() const = 0;
            virtual const NetObjOwnerPtr<T>& at(uint32_t index) const = 0;
            virtual bool empty() const = 0;
            virtual ~NetObjOrderedList() {}
        protected:
            virtual void insert(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::shared_ptr<NetServer::ClientData>& clientInserting, uint32_t posToInsertAt, NetObjOwnerPtr<T> newObj) = 0;
            virtual void erase_by_index(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, uint32_t indexToErase) = 0;
            virtual uint32_t erase_by_ptr(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const NetObjTemporaryPtr<T>& p) = 0;
            virtual void write_constructor(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryOutputArchive& a) = 0;
            virtual void read_update(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) = 0;
            virtual void read_constructor(NetObjManager* objMan, const std::shared_ptr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) = 0;
        private:
            template <typename S> friend void register_ordered_list_class(NetObjManagerTypeList& t);
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
            virtual bool contains(const NetObjTemporaryPtr<T>& p) const override {
                return idToDataMap.contains(p.get_net_id());
            }
            virtual uint32_t size() const override {
                return data.size();
            }
            virtual const NetObjOwnerPtr<T>& at(uint32_t index) const override {
                return data[index]->obj;
            }
            virtual bool empty() const override {
                return data.empty();
            }
        protected:
            virtual void insert(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::shared_ptr<NetServer::ClientData>& clientInserting, uint32_t posToInsertAt, NetObjOwnerPtr<T> newObj) override {
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
            }
            virtual void erase_by_index(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, uint32_t indexToErase) override {
                if(data.empty())
                    return;
                indexToErase = std::min<uint32_t>(indexToErase, data.size() - 1);
                l.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [indexToErase](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::ERASE_SINGLE, indexToErase);
                });
                NetObjID idToErase = data[indexToErase]->obj.get_net_id();
                data.erase(data.begin() + indexToErase);
                idToDataMap.erase(idToErase);
                set_positions_for_object_info_vector<T>(data, indexToErase);
            }
            virtual uint32_t erase_by_ptr(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const NetObjTemporaryPtr<T>& p) override {
                if(data.empty())
                    return std::numeric_limits<uint32_t>::max();
                auto it = idToDataMap.find(p.get_net_id());
                if(it == idToDataMap.end())
                    return std::numeric_limits<uint32_t>::max();
                uint32_t indexToErase = it->second->pos;
                data.erase(data.begin() + indexToErase);
                idToDataMap.erase(it);
                l.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [indexToErase](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::ERASE_SINGLE, indexToErase);
                });
                set_positions_for_object_info_vector<T>(data, indexToErase);
                return indexToErase;
            }
            virtual void write_constructor(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryOutputArchive& a) override {
                a(static_cast<uint32_t>(data.size()));
                for(size_t i = 0; i < data.size(); i++)
                    data[i]->obj.write_create_message(a);
            }
            virtual void read_constructor(NetObjManager* objMan, const std::shared_ptr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) override {
                uint32_t constructedSize;
                a(constructedSize);
                data.clear();
                idToDataMap.clear();
                for(uint32_t i = 0; i < constructedSize; i++) {
                    data.emplace_back(std::make_shared<NetObjOrderedListObjectInfo<T>>(objMan->template read_create_message<T>(a, c), static_cast<uint32_t>(data.size())));
                    idToDataMap.emplace(data.back()->obj.get_net_id(), data.back());
                }
                // Dont send the object data to all clients in the constructor, as the function that called read_create_message is also the one that will decide where to send the data of the constructed object
                // Example: the INSERT_SINGLE command in read_update, if it was inserting another NetObjOrderedList, would first construct the list using read_create_message, then it will call insert() to send it to clients
            }
            virtual void read_update(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) override {
                ObjPtrOrderedListCommand_CtoS command;
                a(command);
                switch(command) {
                    case ObjPtrOrderedListCommand_CtoS::INSERT_SINGLE:
                        uint32_t newPos;
                        a(newPos);
                        insert(l, c, newPos, l.get_obj_man()->template read_create_message<T>(a, c));
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
            virtual bool contains(const NetObjTemporaryPtr<T>& p) const override {
                return clientIdToDataMap.contains(p.get_net_id());
            }
            virtual uint32_t size() const override {
                return clientData.size();
            }
            virtual const NetObjOwnerPtr<T>& at(uint32_t index) const override {
                return clientData[index]->obj;
            }
            virtual bool empty() const override {
                return clientData.empty();
            }
        protected:
            virtual void insert(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const std::shared_ptr<NetServer::ClientData>&, uint32_t posToInsertAt, NetObjOwnerPtr<T> newObj) {
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
                clientIdToDataMap.erase(p->obj.get_net_id());
                clientData.erase(clientData.begin() + iToErase);
                set_positions_for_object_info_vector<T>(clientData, iToErase);
            }
            virtual uint32_t erase_by_ptr(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, const NetObjTemporaryPtr<T>& p) override {
                if(clientData.empty())
                    return std::numeric_limits<uint32_t>::max();
                l.send_client_update(RELIABLE_COMMAND_CHANNEL, [&p](const NetObjTemporaryPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_CtoS::ERASE_SINGLE, p.get_net_id());
                });
                auto it = clientIdToDataMap.find(p.get_net_id());
                uint32_t iToErase = it->second->pos;
                auto toRet = clientData[iToErase];
                clientIdToDataMap.erase(it);
                clientData.erase(clientData.begin() + iToErase);
                set_positions_for_object_info_vector<T>(clientData, iToErase);
                return iToErase;
            }
            virtual void write_constructor(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryOutputArchive& a) override {
                a(static_cast<uint32_t>(clientData.size()));
                for(size_t i = 0; i < clientData.size(); i++)
                    clientData[i]->obj.write_create_message(a);
            }
            virtual void read_constructor(NetObjManager* objMan, const std::shared_ptr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) override {
                uint32_t constructedSize;
                a(constructedSize);
                clientData.clear();
                serverData.clear();
                clientIdToDataMap.clear();
                for(uint32_t i = 0; i < constructedSize; i++) {
                    clientData.emplace_back(std::make_shared<NetObjOrderedListObjectInfo<T>>(objMan->template read_create_message<T>(a, nullptr), static_cast<uint32_t>(serverData.size())));
                    clientIdToDataMap.emplace_back(clientData.back()->obj.get_net_id(), serverData.back());
                    serverData.emplace_back(clientData.back()->obj.get_net_id());
                }
            }
            virtual void read_update(const NetObjTemporaryPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) override {
                ObjPtrOrderedListCommand_StoC c;
                a(c);
                switch(c) {
                    case ObjPtrOrderedListCommand_StoC::INSERT_SINGLE_REFERENCE: {
                        NetObjID newObjID;
                        uint32_t newServerPos;
                        a(newServerPos, newObjID);
                        serverData.insert(serverData.begin() + newServerPos, newObjID);
                        clientJustInsertedSyncIgnore.erase(newObjID);
                        sync();
                        break;
                    }
                    case ObjPtrOrderedListCommand_StoC::INSERT_SINGLE_CONSTRUCT: {
                        uint32_t newServerPos;
                        a(newServerPos);
                        auto objInfoToInsert = std::make_shared<NetObjOrderedListObjectInfo<T>>(l.get_obj_man()->template read_create_message<T>(a, nullptr), 0);
                        serverData.insert(serverData.begin() + newServerPos, objInfoToInsert->obj.get_net_id());
                        clientJustInsertedSyncIgnore.erase(objInfoToInsert->obj.get_net_id());
                        sync();
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
                            clientIdToDataMap.erase(it);
                            clientJustInsertedSyncIgnore.erase(objID);
                            clientData.erase(clientData.begin() + clientPosToErase);
                            set_positions_for_object_info_vector<T>(clientData, clientPosToErase);
                        }
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

                set_positions_for_object_info_vector<T>(clientData);
            }

            std::vector<NetObjID> serverData;
            std::vector<NetObjOrderedListObjectInfoPtr<T>> clientData;
            std::unordered_map<NetObjID, NetObjOrderedListObjectInfoPtr<T>> clientIdToDataMap;
            std::unordered_map<NetObjID, NetObjOrderedListObjectInfoPtr<T>> clientJustInsertedSyncIgnore;
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
