#pragma once
#include "NetObjID.hpp"
#include "NetObjManager.hpp"
#include "NetObjOwnerPtr.decl.hpp"
#include "NetObjTemporaryPtr.decl.hpp"
#include <memory>

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
            NetObjWeakPtr(const NetObjOwnerPtr<T>& other):
                objMan(other.objMan),
                id(other.id)
            {}
            NetObjWeakPtr(const NetObjTemporaryPtr<T>& other):
                objMan(other.objMan),
                id(other.id)
            {}
            NetObjWeakPtr& operator=(const NetObjOwnerPtr<T>& other) {
                objMan = other.objMan;
                id = other.id;
                return *this;
            }
            NetObjWeakPtr& operator=(const NetObjTemporaryPtr<T>& other) {
                objMan = other.objMan;
                id = other.id;
                return *this;
            }
            NetObjWeakPtr& operator=(const NetObjWeakPtr& other) {
                objMan = other.objMan;
                id = other.id;
                return *this;
            }
            template <typename S> NetObjWeakPtr<S> cast() const {
                return NetObjWeakPtr<S>(objMan, id);
            }
            bool expired() const {
                return !objMan || objMan->objectData.contains(id);
            }
            NetObjTemporaryPtr<T> lock() const {
                if(!objMan)
                    return NetObjTemporaryPtr<T>();
                auto it = objMan->objectData.find(id);
                if(it == objMan->objectData.end())
                    return NetObjTemporaryPtr<T>();
                return NetObjTemporaryPtr<T>(objMan, id, static_cast<T*>(it->second.p));
            }
            void reset() {
                objMan = nullptr;
                id = NetObjID({0, 0});
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

            NetObjManager* objMan;
            NetObjID id;
    };
}
