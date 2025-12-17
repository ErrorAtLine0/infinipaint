#pragma once
#include "Helpers/Networking/NetLibrary.hpp"
#include "Helpers/NetworkingObjects/NetObjOwnerPtr.decl.hpp"
#include "NetObjOwnerPtr.hpp"
#include "NetObjTemporaryPtr.hpp"
#include "NetObjWeakPtr.hpp"
#include "NetObjID.hpp"
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/unordered_set.hpp>
#include "NetObjManagerTypeList.hpp"
#include <limits>

namespace NetworkingObjects {
    enum class ObjPtrUnorderedSetCommand_StoC : uint8_t {
        INSERT_SINGLE_CONSTRUCT,
        ERASE_SINGLE
    };

    enum class ObjPtrUnorderedSetCommand_CtoS : uint8_t {
        INSERT_SINGLE,
        ERASE_SINGLE
    };

    template <typename T> class NetObjUnorderedSet {
        public:
            // Don't use this function unless you're sure that class T isn't a base class
            template <typename ...Args> NetObjTemporaryPtr<T> emplace_direct(const NetObjTemporaryPtr<NetObjUnorderedSet<T>>& l, Args&&... items) {
                NetObjOwnerPtr<T> newObj = l.get_obj_man()->template make_obj_direct<T>(items...);
                return l->insert(l, nullptr, std::move(newObj));
            }
            static NetObjTemporaryPtr<T> insert_and_send_create(const NetObjTemporaryPtr<NetObjUnorderedSet<T>>& l, T* newObj) {
                NetObjOwnerPtr<T> newObjOwner = l.get_obj_man()->template make_obj_from_ptr<T>(newObj);
                return l->insert(l, nullptr, std::move(newObjOwner));
            }
            static bool erase(const NetObjTemporaryPtr<NetObjUnorderedSet<T>>& l, const NetObjID& id) {
                return l->erase_by_id(l, id);
            }

            typedef std::function<void(const NetObjOwnerPtr<T>& objPtr)> InsertCallback;
            typedef std::function<void(const NetObjOwnerPtr<T>& objPtr)> EraseCallback;

            void set_insert_callback(const InsertCallback& func) {
                insertCallback = func;
            }
            void set_erase_callback(const EraseCallback& func) {
                eraseCallback = func;
            }

            bool contains(const NetObjID& p) const {
                return std::find_if(data.begin(), data.end(), [&p](const NetObjOwnerPtr<T>& o){ return o.get_net_id() == p; }) != data.end();
            }
            const std::vector<NetObjOwnerPtr<T>>& get_data() const {
                return data;
            }
            uint32_t size() const {
                return data.size();
            }
            bool empty() const {
                return data.empty();
            }
            virtual ~NetObjUnorderedSet() {}
        protected:
            NetObjTemporaryPtr<T> insert(const NetObjTemporaryPtr<NetObjUnorderedSet<T>>& l, const std::shared_ptr<NetServer::ClientData>& clientInserting, NetObjOwnerPtr<T> newObj) {
                auto& objAdded = this->data.emplace_back(std::move(newObj));
                insert_network(l, clientInserting, objAdded);
                call_insert_callback(objAdded);
                return objAdded;
            }

            bool erase_by_id(const NetObjTemporaryPtr<NetObjUnorderedSet<T>>& l, const NetObjID& id) {
                auto it = std::find_if(this->data.begin(), this->data.end(), [&id](const NetObjOwnerPtr<T>& o){ return o.get_net_id() == id; });
                if(it == this->data.end())
                    return false;
                call_erase_callback(*it);
                erase_network(l, id);
                data.erase(it);
                return true;
            }

            virtual void insert_network(const NetObjTemporaryPtr<NetObjUnorderedSet<T>>& l, const std::shared_ptr<NetServer::ClientData>& clientInserting, const NetObjOwnerPtr<T>& newObj) const = 0;
            virtual void erase_network(const NetObjTemporaryPtr<NetObjUnorderedSet<T>>& l, const NetObjID& id) const = 0;
            virtual void read_update(const NetObjTemporaryPtr<NetObjUnorderedSet<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>&) = 0;

            void write_constructor(const NetObjTemporaryPtr<NetObjUnorderedSet<T>>& l, cereal::PortableBinaryOutputArchive& a) {
                a(static_cast<uint32_t>(data.size()));
                for(size_t i = 0; i < data.size(); i++)
                    data[i].write_create_message(a);

            }
            void read_constructor(const NetObjTemporaryPtr<NetObjUnorderedSet<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) {
                uint32_t newDataSize;
                a(newDataSize);
                for(size_t i = 0; i < newDataSize; i++)
                    data.emplace_back(l.get_obj_man()->template read_create_message<T>(a, c));
            }

            // Call after inserting
            void call_insert_callback(const NetObjOwnerPtr<T>& objPtr) {
                if(insertCallback)
                    insertCallback(objPtr);
            }

            // Call before erasing
            void call_erase_callback(const NetObjOwnerPtr<T>& objPtr) {
                if(eraseCallback)
                    eraseCallback(objPtr);
            }

            std::vector<NetObjOwnerPtr<T>> data;

        private:
            InsertCallback insertCallback;
            EraseCallback eraseCallback;

            template <typename S> friend void register_unordered_set_class(NetObjManager& t);
            static void write_constructor_func(const NetObjTemporaryPtr<NetObjUnorderedSet<T>>& l, cereal::PortableBinaryOutputArchive& a) {
                l->write_constructor(l, a);
            }
            static void read_constructor_func(const NetObjTemporaryPtr<NetObjUnorderedSet<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) {
                l->read_constructor(l, a, c);
            }
            static void read_update_func(const NetObjTemporaryPtr<NetObjUnorderedSet<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) {
                l->read_update(l, a, c);
            }
    };

    template <typename T> class NetObjUnorderedSetServer : public NetObjUnorderedSet<T> {
        public:
            NetObjUnorderedSetServer() {}
        protected:
            virtual void insert_network(const NetObjTemporaryPtr<NetObjUnorderedSet<T>>& l, const std::shared_ptr<NetServer::ClientData>& clientInserting, const NetObjOwnerPtr<T>& newObj) const override {
                l.send_server_update_to_all_clients_except(clientInserting, RELIABLE_COMMAND_CHANNEL, [&newObj](const NetObjTemporaryPtr<NetObjUnorderedSet<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrUnorderedSetCommand_StoC::INSERT_SINGLE_CONSTRUCT);
                    newObj.write_create_message(a);
                });
            }
            virtual void erase_network(const NetObjTemporaryPtr<NetObjUnorderedSet<T>>& l, const NetObjID& id) const override {
                l.send_server_update_to_all_clients(RELIABLE_COMMAND_CHANNEL, [id](const NetObjTemporaryPtr<NetObjUnorderedSet<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrUnorderedSetCommand_StoC::ERASE_SINGLE, id);
                });
            }
            virtual void read_update(const NetObjTemporaryPtr<NetObjUnorderedSet<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) override {
                ObjPtrUnorderedSetCommand_CtoS command;
                a(command);
                switch(command) {
                    case ObjPtrUnorderedSetCommand_CtoS::INSERT_SINGLE: {
                        NetObjUnorderedSet<T>::insert(l, c, l.get_obj_man()->template read_create_message<T>(a, c));
                        break;
                    }
                    case ObjPtrUnorderedSetCommand_CtoS::ERASE_SINGLE: {
                        NetObjID idToErase;
                        a(idToErase);
                        NetObjUnorderedSet<T>::erase_by_id(l, idToErase);
                        break;
                    }
                }
            }
    };

    template <typename T> class NetObjUnorderedSetClient : public NetObjUnorderedSet<T> {
        public:
            NetObjUnorderedSetClient() {}
        protected:
            virtual void insert_network(const NetObjTemporaryPtr<NetObjUnorderedSet<T>>& l, const std::shared_ptr<NetServer::ClientData>&, const NetObjOwnerPtr<T>& newObj) const override {
                l.send_client_update(RELIABLE_COMMAND_CHANNEL, [&newObj](const NetObjTemporaryPtr<NetObjUnorderedSet<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrUnorderedSetCommand_CtoS::INSERT_SINGLE);
                    newObj.write_create_message(a);
                });
            }
            virtual void erase_network(const NetObjTemporaryPtr<NetObjUnorderedSet<T>>& l, const NetObjID& id) const override {
                l.send_client_update(RELIABLE_COMMAND_CHANNEL, [id](const NetObjTemporaryPtr<NetObjUnorderedSet<T>>&, cereal::PortableBinaryOutputArchive& a) {
                    a(ObjPtrUnorderedSetCommand_CtoS::ERASE_SINGLE, id);
                });
            }
            virtual void read_update(const NetObjTemporaryPtr<NetObjUnorderedSet<T>>& l, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) override {
                ObjPtrUnorderedSetCommand_StoC command;
                a(command);
                switch(command) {
                    case ObjPtrUnorderedSetCommand_StoC::INSERT_SINGLE_CONSTRUCT: {
                        this->data.emplace_back(l.get_obj_man()->template read_create_message<T>(a, c));
                        break;
                    }
                    case ObjPtrUnorderedSetCommand_StoC::ERASE_SINGLE: {
                        NetObjID id;
                        a(id);
                        auto it = std::find_if(this->data.begin(), this->data.end(), [&id](const NetObjOwnerPtr<T>& o){ return o.get_net_id() == id; });
                        if(it != this->data.end())
                            this->data.erase(it);
                        break;
                    }
                }
            }
    };

    template <typename S> void register_unordered_set_class(NetObjManager& objMan) {
        objMan.register_class<NetObjUnorderedSet<S>, NetObjUnorderedSet<S>, NetObjUnorderedSetClient<S>, NetObjUnorderedSetServer<S>>({
            .writeConstructorFuncClient = NetObjUnorderedSet<S>::write_constructor_func,
            .readConstructorFuncClient = NetObjUnorderedSet<S>::read_constructor_func,
            .readUpdateFuncClient = NetObjUnorderedSet<S>::read_update_func,
            .writeConstructorFuncServer = NetObjUnorderedSet<S>::write_constructor_func,
            .readConstructorFuncServer = NetObjUnorderedSet<S>::read_constructor_func,
            .readUpdateFuncServer = NetObjUnorderedSet<S>::read_update_func,
        });
    }
}
