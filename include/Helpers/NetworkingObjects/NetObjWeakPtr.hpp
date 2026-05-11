/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
                return (!objMan) || (!objMan->objectData.contains(id));
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
