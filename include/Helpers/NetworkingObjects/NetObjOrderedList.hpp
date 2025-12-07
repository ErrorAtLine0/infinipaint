#pragma once
#include "Helpers/Networking/NetLibrary.hpp"
#include "NetObjPtr.hpp"
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
            template <typename... Args> static NetObjPtr<T> emplace_back(const NetObjPtr<NetObjOrderedList<T>>& l, Args&&... args) {
                NetObjPtr<T> newObj = l.get_obj_man()->template make_obj<T>(args...);
                return push_back_and_send_create(l, newObj);
            }
            static NetObjPtr<T> push_back_and_send_create(const NetObjPtr<NetObjOrderedList<T>>& l, const NetObjPtr<T>& newObj) {
                l->push_back(l, newObj);
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
                l->erase_by_ptr(l, p);
            }
            virtual uint32_t size() const = 0;
            virtual const std::vector<NetObjOrderedListObjectInfoPtr<T>>& get_data() const = 0;
        protected:
            virtual void push_back(const NetObjPtr<NetObjOrderedList<T>>& l, const NetObjPtr<T>& newObj) = 0;
            virtual void insert(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t posToInsertAt, const NetObjPtr<T>& newObj) = 0;
            virtual void erase_by_index(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t indexToErase) = 0;
            virtual void erase_by_ptr(const NetObjPtr<NetObjOrderedList<T>>& l, const NetObjOrderedListObjectInfoPtr<T>& p) = 0;
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
            virtual uint32_t size() const override {
                return data.size();
            }
            virtual const std::vector<NetObjOrderedListObjectInfoPtr<T>>& get_data() const override {
                return data;
            }
        protected:
            virtual void push_back(const NetObjPtr<NetObjOrderedList<T>>& l, const NetObjPtr<T>& newObj) override {
                auto& newObjInfoPtr = data.emplace_back(std::make_shared<NetObjOrderedListObjectInfo<T>>(newObj, static_cast<uint32_t>(data.size()), false));
                l.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [&newObjInfoPtr](const NetObjPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::INSERT_SINGLE, newObjInfoPtr->pos);
                    newObjInfoPtr->obj.write_create_message(a);
                });
            }
            virtual void insert(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t posToInsertAt, const NetObjPtr<T>& newObj) override {
                posToInsertAt = std::min<uint32_t>(posToInsertAt, data.size());
                auto& newObjInfoPtr = *data.insert(data.begin() + posToInsertAt, std::make_shared<NetObjOrderedListObjectInfo<T>>(newObj, posToInsertAt, false));
                set_positions_for_object_info_vector<T>(data, posToInsertAt);
                l.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [&newObjInfoPtr](const NetObjPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::INSERT_SINGLE, newObjInfoPtr->pos);
                    newObjInfoPtr->obj.write_create_message(a);
                });
            }
            virtual void erase_by_index(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t indexToErase) override {
                NetObjOrderedListObjectInfoPtr<T>& objToErase = data[indexToErase];
                l.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [indexToErase](const NetObjPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::ERASE_SINGLE, indexToErase);
                });
                data.erase(data.begin() + indexToErase);
                set_positions_for_object_info_vector<T>(data, indexToErase);
            }
            virtual void erase_by_ptr(const NetObjPtr<NetObjOrderedList<T>>& l, const NetObjOrderedListObjectInfoPtr<T>& p) override {
                l.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [indexToErase = p->pos](const NetObjPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_StoC::ERASE_SINGLE, indexToErase);
                });
                data.erase(data.begin() + p->pos);
                set_positions_for_object_info_vector<T>(data, p->pos);
            }
            virtual void write_constructor(const NetObjPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryOutputArchive& a) override {
            }
            virtual void read_constructor(const NetObjPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) override {
            }
            virtual void read_update(const NetObjPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) override {
                ObjPtrOrderedListCommand_CtoS command;
                a(command);
                switch(command) {
//cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& clientReceivedFrom
                    case ObjPtrOrderedListCommand_CtoS::INSERT_SINGLE:
                        uint32_t newPos;
                        a(newPos);
                        insert(l, newPos, l.get_obj_man()->template read_create_message<T>(a, c));
                        break;
                    case ObjPtrOrderedListCommand_CtoS::ERASE_SINGLE:
                        NetObjID idToErase;
                        a(idToErase);
                        insert(l, newPos, l.get_obj_man()->template read_create_message<T>(a, c));
                        break;
                }
            }
        private:
            std::vector<NetObjOrderedListObjectInfoPtr<T>> data;
    };

    template <typename T> class NetObjOrderedListClient : public NetObjOrderedList<T> {
        public:
            virtual uint32_t size() const override {
                return clientData.size();
            }
            virtual const std::vector<NetObjOrderedListObjectInfoPtr<T>>& get_data() const override {
                return clientData;
            }
        protected:
            virtual void push_back(const NetObjPtr<NetObjOrderedList<T>>& l, const NetObjPtr<T>& newObj) override {
                auto& newObjInfoPtr = clientData.emplace_back(std::make_shared<NetObjOrderedListObjectInfo<T>>(newObj, static_cast<uint32_t>(clientData.size()), true));
                l.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [&newObjInfoPtr](const NetObjPtr<NetObjOrderedList<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrOrderedListCommand_CtoS::INSERT_SINGLE, newObjInfoPtr->pos);
                    newObjInfoPtr->obj.write_create_message(a);
                });
            }
            virtual void insert(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t posToInsertAt, const NetObjPtr<T>& newObj) {
            }
            virtual void erase_by_index(const NetObjPtr<NetObjOrderedList<T>>& l, uint32_t indexToErase) override {
            }
            virtual void erase_by_ptr(const NetObjPtr<NetObjOrderedList<T>>& l, const NetObjOrderedListObjectInfoPtr<T>& p) override {
            }
            virtual void write_constructor(const NetObjPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryOutputArchive& a) override {
            }
            virtual void read_constructor(const NetObjPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) override {
            }
            virtual void read_update(const NetObjPtr<NetObjOrderedList<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) override {
                ObjPtrOrderedListCommand_StoC c;
                a(c);
                switch(c) {
                    case ObjPtrOrderedListCommand_StoC::INSERT_SINGLE:
                        break;
                    case ObjPtrOrderedListCommand_StoC::ERASE_SINGLE:
                        break;
                }
            }
        private:
            std::vector<NetObjOrderedListObjectInfoPtr<T>> serverData;
            std::vector<NetObjOrderedListObjectInfoPtr<T>> clientData;
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
