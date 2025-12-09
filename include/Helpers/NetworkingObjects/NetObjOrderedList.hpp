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

    template <typename T> class NetObjOrderedListObjectInfo {
        public: 
            NetObjOrderedListObjectInfo(const NetObjPtr<T>& initObj, uint32_t initPos, bool initSyncIgnore):
                obj(initObj),
                pos(initPos),
                syncIgnore(initSyncIgnore)
            {}
            uint32_t get_pos() const { return pos; }
            const NetObjPtr<T>& get_obj() const { return obj; }
        private:
            NetObjPtr<T> obj;
            uint32_t pos;
            bool syncIgnore;
            template <typename S> friend class NetObjOrderedList;
            template <typename S> friend class NetObjOrderedListServer;
            template <typename S> friend class NetObjOrderedListClient;
            template <typename S> friend void set_positions_for_object_info_vector(std::vector<std::shared_ptr<NetObjOrderedListObjectInfo<S>>>& v, uint32_t i);
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
            static NetObjPtr<T> push_back_and_send_create(const NetObjPtr<NetObjOrderedList<T>>& l, const NetObjPtr<T>& newObj) {
                l->insert(l, l->size(), newObj);
                return newObj;
            }
            static NetObjPtr<T> insert_and_send_create(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t posToInsertAt, const NetObjPtr<T>& newObj) {
                l->insert(l, posToInsertAt, newObj);
                return newObj;
            }
            static void erase(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t indexToErase) {
                l->erase_by_index(l, indexToErase);
            }
            static void erase(const NetObjPtr<NetObjOrderedList<T>>& l, const NetObjOrderedListObjectInfoPtr<T>& p) {
                l->erase_by_index(l, p->pos);
            }
            virtual uint32_t size() const = 0;
            virtual const std::vector<NetObjOrderedListObjectInfoPtr<T>>& get_data() const = 0;
            virtual ~NetObjOrderedList() {}
        protected:
            virtual void insert(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t posToInsertAt, const NetObjPtr<T>& newObj) = 0;
            virtual void erase_by_index(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t indexToErase) = 0;
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
            virtual const std::vector<NetObjOrderedListObjectInfoPtr<T>>& get_data() const override {
                return data;
            }
        protected:
            virtual void insert(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t posToInsertAt, const NetObjPtr<T>& newObj) override {
                uint32_t actualPosToInsertAt = posToInsertAt = std::min<uint32_t>(posToInsertAt, data.size());

                // Find duplicate. If found, erase the duplicate before inserting
                auto itObjAlreadyInData = idToDataMap.find(newObj.get_net_id());
                if(itObjAlreadyInData != idToDataMap.end()) {
                    uint32_t posToErase = itObjAlreadyInData->second->pos;
                    data.erase(data.begin() + posToErase);
                    idToDataMap.erase(itObjAlreadyInData);
                    if(posToErase < actualPosToInsertAt) // Shift insert position if an element was erased before the insert position
                        actualPosToInsertAt--;
                }

                // Insert the new object
                auto newObjInfoPtr = std::make_shared<NetObjOrderedListObjectInfo<T>>(newObj, actualPosToInsertAt, false);
                data.insert(data.begin() + actualPosToInsertAt, newObjInfoPtr);
                idToDataMap.emplace(newObjInfoPtr->obj.get_net_id(), newObjInfoPtr);
                l.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [&newObjInfoPtr, posToInsertAt](const NetObjPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::INSERT_SINGLE, posToInsertAt); // Dont send actual pos to insert at, since client will check for duplicates on its own and shift the index if necessary
                    newObjInfoPtr->obj.write_create_message(a);
                });
                set_positions_for_object_info_vector<T>(data, actualPosToInsertAt);
            }
            virtual void erase_by_index(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t indexToErase) override {
                if(data.empty())
                    return;
                indexToErase = std::min<uint32_t>(indexToErase, data.size() - 1);
                l.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [indexToErase](const NetObjPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::ERASE_SINGLE, indexToErase);
                });
                NetObjID idToErase = data[indexToErase]->obj.get_net_id();
                data.erase(data.begin() + indexToErase);
                idToDataMap.erase(idToErase);
                set_positions_for_object_info_vector<T>(data, indexToErase);
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
            virtual const std::vector<NetObjOrderedListObjectInfoPtr<T>>& get_data() const override {
                return clientData;
            }
        protected:
            virtual void insert(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t posToInsertAt, const NetObjPtr<T>& newObj) {
                uint32_t actualPosToInsertAt = posToInsertAt = std::min<uint32_t>(posToInsertAt, clientData.size());

                // Find duplicate. If found, erase the duplicate before inserting
                auto itObjAlreadyInData = clientIdToDataMap.find(newObj.get_net_id());
                if(itObjAlreadyInData != clientIdToDataMap.end()) {
                    uint32_t posToErase = itObjAlreadyInData->second->pos;
                    clientData.erase(clientData.begin() + posToErase);
                    clientIdToDataMap.erase(itObjAlreadyInData);
                    if(posToErase < actualPosToInsertAt) // Shift insert position if an element was erased before the insert position
                        actualPosToInsertAt--;
                }

                // Insert the new object
                auto newObjInfoPtr = std::make_shared<NetObjOrderedListObjectInfo<T>>(newObj, actualPosToInsertAt, false);
                clientData.insert(clientData.begin() + actualPosToInsertAt, newObjInfoPtr);
                clientIdToDataMap.emplace(newObjInfoPtr->obj.get_net_id(), newObjInfoPtr);
                l.send_client_update(RELIABLE_COMMAND_CHANNEL, [&newObjInfoPtr, posToInsertAt](const NetObjPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_CtoS::INSERT_SINGLE, posToInsertAt); // Don't put actual pos to insert at, since the server will check for duplicates and might shift the index on its own
                    newObjInfoPtr->obj.write_create_message(a);
                });
                set_positions_for_object_info_vector<T>(clientData, actualPosToInsertAt);
            }
            virtual void erase_by_index(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t indexToErase) override {
                if(clientData.empty())
                    return;
                indexToErase = std::min<uint32_t>(indexToErase, clientData.size() - 1);
                auto& p = clientData[indexToErase];
                l.send_client_update(RELIABLE_COMMAND_CHANNEL, [&p](const NetObjPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_CtoS::ERASE_SINGLE, p->obj.get_net_id());
                });
                p->syncIgnore = true;
                uint32_t iToErase = p->pos;
                clientIdToDataMap.erase(p->obj.get_net_id());
                clientData.erase(clientData.begin() + iToErase);
                set_positions_for_object_info_vector<T>(clientData, iToErase);
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
                        uint32_t newPos;
                        a(newPos);
                        auto toInsert = std::make_shared<NetObjOrderedListObjectInfo<T>>(l.get_obj_man()->template read_create_message<T>(a, nullptr), newPos, false);
                        auto it = serverIdToDataMap.find(toInsert->obj.get_net_id());
                        if(it != serverIdToDataMap.end()) {
                            uint32_t posToErase = it->second->pos;
                            serverData.erase(serverData.begin() + posToErase);
                            serverIdToDataMap.erase(it);
                            if(posToErase < newPos)
                                newPos--; // Shift if there was a duplicate before this
                        }
                        serverIdToDataMap.emplace(toInsert->obj.get_net_id(), toInsert);
                        serverData.insert(serverData.begin() + newPos, toInsert);
                        break;
                    }
                    case ObjPtrOrderedListCommand_StoC::ERASE_SINGLE: {
                        uint32_t posToErase;
                        a(posToErase);
                        NetObjID objID = serverData[posToErase]->obj.get_net_id();
                        serverData.erase(serverData.begin() + posToErase);
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
