#pragma once
#include "NetObjID.hpp"
#include "NetObjManager.hpp"
#include "NetObjPtr.decl.hpp"

namespace NetworkingObjects {
    template <typename T> class NetObjWeakPtr {
        public:
            NetObjWeakPtr():
                objMan(nullptr),
                id({0, 0})
            {}
            NetObjWeakPtr(const NetObjWeakPtr& other):
                objMan(other.objMan),
                id(other.id)
            {}
            NetObjWeakPtr(const NetObjPtr<T>& other):
                objMan(other.objMan),
                id(other.id)
            {}
            NetObjWeakPtr& operator=(const NetObjWeakPtr& other) {
                objMan = other.objMan;
                id = other.id;
            }
            template <typename S> NetObjWeakPtr<S> cast() const {
                return NetObjWeakPtr<S>(objMan, id);
            }
            bool expired() const {
                if(!objMan)
                    return true;
                auto it = objMan->objectData.find(id);
                return it == objMan->objectData.end();
            }
            NetObjPtr<T> lock() const {
                auto it = objMan->objectData.find(id);
                if(it == objMan->objectData.end())
                    return NetObjPtr<T>();
                else
                    return NetObjPtr<T>(objMan, id, std::static_pointer_cast<T>(it->second.p));
            }
            NetObjManager* get_obj_man() const {
                return objMan;
            }
            NetObjID get_net_id() const {
                return id;
            }
            bool operator!=(const NetObjWeakPtr<T>& rhs) const {
                return objMan != rhs.objMan || id != rhs.id;
            }
            bool operator==(const NetObjWeakPtr<T>& rhs) const {
                return objMan == rhs.objMan && id == rhs.id;
            }
        private:
            NetObjWeakPtr(NetObjManager* initObjMan, NetObjID initID):
                objMan(initObjMan),
                id(initID)
            {}

            template <typename S> friend class NetObjPtr;
            NetObjManager* objMan;
            NetObjID id;
    };
}
