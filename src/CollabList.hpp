#pragma once
#include "SharedTypes.hpp"

template <typename T, typename IDType> class CollabList {
    public:
        CollabList(const std::function<IDType()>& initGetNewIDFunc):
            getNewIDFunc(initGetNewIDFunc)
        {}

        struct ObjectInfo {
            bool syncIgnore;
            IDType id;
            T obj;
            uint64_t pos;
        };

        typedef std::shared_ptr<ObjectInfo> ObjectInfoPtr;

        const std::vector<std::shared_ptr<ObjectInfo>>& client_list() const { return clientSideList; }
        std::vector<std::shared_ptr<ObjectInfo>>& client_list() { return clientSideList; }
        const std::vector<std::shared_ptr<ObjectInfo>>& server_list() const { return serverSideList; }

        std::function<void()> updateCallback;
        std::function<void(const ObjectInfoPtr& c)> clientInsertCallback;
        std::function<void(const ObjectInfoPtr& c)> clientEraseCallback;
        std::function<void(const std::vector<ObjectInfoPtr>& c)> clientInsertOrderedVectorCallback;
        std::function<void(uint64_t oldPos)> clientServerLastPosShiftCallback;

        void set_server_from_client_list() {
            serverSideList = clientSideList;
            for(auto& obj : serverSideList)
                obj->syncIgnore = false;
        }

        ObjectInfoPtr get_item_by_id(IDType id) {
            auto it = idToObjectMap.find(id);
            return (idToObjectMap.end() == it) ? nullptr : it->second;
        }

        void client_erase_if(const std::function<bool(uint64_t, const std::shared_ptr<ObjectInfo>&)>& func) {
            bool erasedSomething = false;
            for(int64_t i = 0; i < (int64_t)clientSideList.size(); i++) {
                if(func((uint64_t)i, clientSideList[i])) {
                    if(clientEraseCallback)
                        clientEraseCallback(clientSideList[i]);
                    clientSideList[i]->obj->collabListInfo.reset();
                    idToObjectMap.erase(clientSideList[i]->id);
                    clientSideList.erase(clientSideList.begin() + i);
                    i--;
                    if(updateCallback)
                        updateCallback();
                    erasedSomething = true;
                }
            }
            if(erasedSomething)
                complete_client_pos_refresh(false);
        }

        ObjectInfoPtr client_insert(uint64_t pos, const T& item) {
            auto newObj(std::make_shared<ObjectInfo>());
            newObj->syncIgnore = true;
            newObj->id = getNewIDFunc();
            newObj->obj = item;
            newObj->pos = std::min<uint64_t>(static_cast<uint64_t>(clientSideList.size()), pos);
            item->collabListInfo = newObj;

            clientSideList.insert(clientSideList.begin() + newObj->pos, newObj);
            idToObjectMap.emplace(newObj->id, newObj);

            client_pos_refresh_after_insert(newObj->pos);

            if(clientInsertCallback)
                clientInsertCallback(newObj);
            if(updateCallback)
                updateCallback();

            return newObj;
        }

        IDType client_insert(const ObjectInfoPtr& objInfoToInsert) {
            objInfoToInsert->syncIgnore = true;
            objInfoToInsert->id = getNewIDFunc();
            objInfoToInsert->pos = std::min<uint64_t>(static_cast<uint64_t>(clientSideList.size()), objInfoToInsert->pos);
            objInfoToInsert->obj->collabListInfo = objInfoToInsert;

            clientSideList.insert(clientSideList.begin() + objInfoToInsert->pos, objInfoToInsert);
            idToObjectMap.emplace(objInfoToInsert->id, objInfoToInsert);

            client_pos_refresh_after_insert(objInfoToInsert->pos);

            if(clientInsertCallback)
                clientInsertCallback(objInfoToInsert);
            if(updateCallback)
                updateCallback();

            return objInfoToInsert->id;
        }

        void client_erase(const ObjectInfoPtr& objToErase) {
            bool erasedSomething = false;
            std::erase_if(clientSideList, [&](const auto& obj) {
                if(obj->obj == objToErase->obj) {
                    if(clientEraseCallback)
                        clientEraseCallback(obj);
                    idToObjectMap.erase(objToErase->id);
                    obj->syncIgnore = true;
                    obj->obj->collabListInfo.reset();
                    erasedSomething = true;
                    return true;
                }
                return false;
            });
            if(erasedSomething) {
                if(updateCallback)
                    updateCallback();
                complete_client_pos_refresh(false);
            }
        }

        void client_erase_set(const std::unordered_set<ObjectInfoPtr>& objSet) {
            bool erasedSomething = false;
            std::erase_if(clientSideList, [&](const auto& obj) {
                if(objSet.contains(obj)) {
                    obj->syncIgnore = true;
                    obj->obj->collabListInfo.reset();
                    idToObjectMap.erase(obj->id);
                    if(clientEraseCallback)
                        clientEraseCallback(obj);
                    erasedSomething = true;
                    return true;
                }
                return false;
            });
            if(erasedSomething) {
                complete_client_pos_refresh(false);
                if(updateCallback)
                    updateCallback();
                for(auto& o : objSet)
                    o->obj->collabListInfo.reset();
            }
        }

        // NOTE: Dont use this function unless you're certain every object in the vector isn't already in the client list
        void client_insert_ordered_vector(const std::vector<ObjectInfoPtr>& objOrderedVector) {
            if(objOrderedVector.empty())
                return;

            uint64_t startPoint = 0;
            objOrderedVector.front()->syncIgnore = true;
            objOrderedVector.front()->id = getNewIDFunc();
            objOrderedVector.front()->obj->collabListInfo = objOrderedVector.front();
            idToObjectMap.emplace(objOrderedVector.front()->id, objOrderedVector.front());

            for(uint64_t i = 1; i < static_cast<uint64_t>(objOrderedVector.size()); i++) {
                auto& obj = objOrderedVector[i];
                obj->syncIgnore = true;
                obj->id = getNewIDFunc();
                obj->obj->collabListInfo = obj;
                idToObjectMap.emplace(obj->id, obj);
                if(obj->pos - objOrderedVector[startPoint]->pos != (i - startPoint)) {
                    uint64_t insertPosition = std::min(clientSideList.size(), objOrderedVector[startPoint]->pos);
                    clientSideList.insert(clientSideList.begin() + insertPosition, objOrderedVector.begin() + startPoint, objOrderedVector.begin() + i);
                    startPoint = i;
                }
            }
            clientSideList.insert(clientSideList.begin() + std::min(clientSideList.size(), objOrderedVector[startPoint]->pos), objOrderedVector.begin() + startPoint, objOrderedVector.end());

            complete_client_pos_refresh(false);
            if(updateCallback)
                updateCallback();
            if(clientInsertOrderedVectorCallback)
                clientInsertOrderedVectorCallback(objOrderedVector);
        }

        std::vector<bool> server_insert_ordered_vector(std::vector<ObjectInfoPtr> objOrderedVector) {
            if(objOrderedVector.empty())
                return std::vector<bool>();

            std::vector<bool> didntExistPreviously(objOrderedVector.size(), true);

            uint64_t startPoint = 0;
            auto foundInClient1 = idToObjectMap.emplace(objOrderedVector.front()->id, objOrderedVector.front());
            if(!foundInClient1.second) { // object already existed
                objOrderedVector.front() = foundInClient1.first->second;
                didntExistPreviously[0] = false;
            }
            objOrderedVector.front()->syncIgnore = false;
            objOrderedVector.front()->obj->collabListInfo = objOrderedVector.front();

            for(uint64_t i = 1; i < static_cast<uint64_t>(objOrderedVector.size()); i++) {
                auto& obj = objOrderedVector[i];

                auto foundInClient = idToObjectMap.emplace(obj->id, obj);
                if(!foundInClient.second) { // object already existed
                    obj = foundInClient.first->second;
                    didntExistPreviously[i] = false;
                }
                obj->syncIgnore = false;
                obj->obj->collabListInfo = obj;

                if(obj->pos - objOrderedVector[startPoint]->pos != (i - startPoint)) {
                    uint64_t insertPosition = std::min(clientSideList.size(), objOrderedVector[startPoint]->pos);
                    clientSideList.insert(clientSideList.begin() + insertPosition, objOrderedVector.begin() + startPoint, objOrderedVector.begin() + i);
                    uint64_t insertPositionServer = std::min(serverSideList.size(), objOrderedVector[startPoint]->pos);
                    serverSideList.insert(serverSideList.begin() + insertPositionServer, objOrderedVector.begin() + startPoint, objOrderedVector.begin() + i);
                    startPoint = i;
                }
            }
            clientSideList.insert(clientSideList.begin() + std::min(clientSideList.size(), objOrderedVector[startPoint]->pos), objOrderedVector.begin() + startPoint, objOrderedVector.end());
            serverSideList.insert(serverSideList.begin() + std::min(serverSideList.size(), objOrderedVector[startPoint]->pos), objOrderedVector.begin() + startPoint, objOrderedVector.end());

            if(clientInsertOrderedVectorCallback)
                clientInsertOrderedVectorCallback(objOrderedVector);

            sync();

            return didntExistPreviously;
        }

        bool server_insert(uint64_t pos, IDType id, const T& item) {
            auto foundInClient = idToObjectMap.find(id);
            if(foundInClient != idToObjectMap.end()) {
                ObjectInfoPtr& o = foundInClient->second;
                serverSideList.insert(serverSideList.begin() + pos, o);
                o->syncIgnore = false;
                sync();
                return false;
            }
            auto newObj(std::make_shared<ObjectInfo>());
            newObj->syncIgnore = false;
            newObj->id = id;
            newObj->obj = item;
            newObj->pos = std::min<uint64_t>(pos, clientSideList.size());
            idToObjectMap.emplace(newObj->id, newObj);
            item->collabListInfo = newObj;

            serverSideList.insert(serverSideList.begin() + pos, newObj);
            clientSideList.insert(clientSideList.begin() + newObj->pos, newObj);

            for(uint64_t i = newObj->pos + 1; i < static_cast<uint64_t>(clientSideList.size()); i++)
                clientSideList[i]->pos++;

            if(clientInsertCallback)
                clientInsertCallback(newObj);

            sync();
            return true;
        }
        void server_erase(IDType id) {
            std::erase_if(serverSideList, [&](const auto& obj) {
                if(obj->id == id) {
                    obj->syncIgnore = false;
                    return true;
                }
                return false;
            });
            std::erase_if(clientSideList, [&](const auto& obj) {
                if(obj->id == id) {
                    obj->syncIgnore = false;
                    idToObjectMap.erase(obj->id);
                    if(clientEraseCallback)
                        clientEraseCallback(obj);
                    obj->obj->collabListInfo.reset();
                    return true;
                }
                return false;
            });
            sync();
        }

        void server_erase_set(const std::unordered_set<ServerClientID>& idsToErase) {
            std::erase_if(serverSideList, [&](const auto& obj) {
                if(idsToErase.contains(obj->id)) {
                    obj->syncIgnore = false;
                    return true;
                }
                return false;
            });
            std::erase_if(clientSideList, [&](const auto& obj) {
                if(idsToErase.contains(obj->id)) {
                    obj->syncIgnore = false;
                    if(clientEraseCallback)
                        clientEraseCallback(obj);
                    idToObjectMap.erase(obj->id);
                    obj->obj->collabListInfo.reset();
                    return true;
                }
                return false;
            });
            sync();
        }

        void init_emplace_back(IDType id, const T& item) {
            auto newObj(std::make_shared<ObjectInfo>());
            newObj->syncIgnore = false;
            newObj->id = id;
            newObj->obj = item;
            item->collabListInfo = newObj;
            idToObjectMap.emplace(newObj->id, newObj);

            serverSideList.emplace_back(newObj);
            clientSideList.emplace_back(newObj);

            newObj->pos = clientSideList.size() - 1;

            if(clientInsertCallback)
                clientInsertCallback(newObj);
            if(updateCallback)
                updateCallback();
        }
    private:
        void complete_client_pos_refresh(bool syncFromServer) {
            uint64_t lastShiftPos = 0;
            bool shiftHappened = false;
            for(uint64_t i = 0; i < clientSideList.size(); i++) {
                auto& c = clientSideList[i];
                if(c->pos != i) {
                    lastShiftPos = std::max(lastShiftPos, c->pos);
                    lastShiftPos = std::max(lastShiftPos, i);
                    shiftHappened = true;
                    c->pos = i;
                }
            }
            if(syncFromServer && shiftHappened && clientServerLastPosShiftCallback)
                clientServerLastPosShiftCallback(lastShiftPos);
        }

        void client_pos_refresh_after_insert(uint64_t posToRefreshAfter) {
            for(uint64_t i = posToRefreshAfter + 1; i < static_cast<uint64_t>(clientSideList.size()); i++)
                clientSideList[i]->pos = i;
        }

        void sync() {
            std::vector<std::pair<uint64_t, ObjectInfoPtr>> ignoredToReinsert;
            for(uint64_t i = 0; i < clientSideList.size(); i++)
                if(clientSideList[i]->syncIgnore)
                    ignoredToReinsert.emplace_back(std::pair<uint64_t, std::shared_ptr<ObjectInfo>>{i, clientSideList[i]});
            clientSideList = serverSideList;
            std::erase_if(clientSideList, [&](const auto& obj) {
                return obj->syncIgnore;
            });
            for(auto& p : ignoredToReinsert) {
                if(p.first > clientSideList.size())
                    clientSideList.emplace_back(p.second);
                else
                    clientSideList.insert(clientSideList.begin() + p.first, p.second);
            }
            if(updateCallback)
                updateCallback();
            complete_client_pos_refresh(true);
        }

        std::function<IDType()> getNewIDFunc;
        std::vector<ObjectInfoPtr> clientSideList;
        std::vector<ObjectInfoPtr> serverSideList;
        std::unordered_map<IDType, ObjectInfoPtr> idToObjectMap;
};
