#include "DrawingProgramLayerManager.hpp"
#include "../../World.hpp"
#include "Helpers/NetworkingObjects/NetObjID.hpp"
#include "Helpers/NetworkingObjects/NetObjOrderedList.hpp"
#include "Helpers/NetworkingObjects/NetObjOwnerPtr.decl.hpp"
#include "Helpers/NetworkingObjects/NetObjTemporaryPtr.decl.hpp"
#include "Helpers/Parallel.hpp"
#include "../../CanvasComponents/CanvasComponentContainer.hpp"
#include "../../CanvasComponents/CanvasComponentAllocator.hpp"
#include "../../CanvasComponents/CanvasComponent.hpp"
#include <chrono>

DrawingProgramLayerManager::DrawingProgramLayerManager(DrawingProgram& drawProg):
    drawP(drawProg), // initialize drawP first for next objects to be initialized properly
    listGUI(*this)
{}

void DrawingProgramLayerManager::server_init_no_file() {
    layerTreeRoot = drawP.world.netObjMan.make_obj_from_ptr<DrawingProgramLayerListItem>(new DrawingProgramLayerListItem(drawP.world.netObjMan, "ROOT", true));
    layerTreeRoot->get_folder().set_component_list_callbacks(*this); // Only need to call set_component_list_callbacks on root, and the rest will get the callbacks set as well
    editingLayer = layerTreeRoot->get_folder().folderList->emplace_back_direct(layerTreeRoot->get_folder().folderList, drawP.world.netObjMan, "First Layer", false)->obj;
}

void DrawingProgramLayerManager::scale_up(const WorldScalar& scaleUpAmount) {
    layerTreeRoot->scale_up(scaleUpAmount);
}

void DrawingProgramLayerManager::write_components_server(cereal::PortableBinaryOutputArchive& a) {
    layerTreeRoot.write_create_message(a);
}

void DrawingProgramLayerManager::read_components_client(cereal::PortableBinaryInputArchive& a) {
    layerTreeRoot = drawP.world.netObjMan.read_create_message<DrawingProgramLayerListItem>(a, nullptr);
    commitUpdateOnComponentInsert = false;
    layerTreeRoot->set_component_list_callbacks(*this);
    commitUpdateOnComponentInsert = true;
    auto flattenedCompList = get_flattened_component_list();
    parallel_loop_container(flattenedCompList, [&drawP = drawP](CanvasComponentContainer::ObjInfo* comp) {
        comp->obj->commit_update_dont_invalidate_cache(drawP);
    });
    drawP.rebuild_cache();
    editingLayer = layerTreeRoot->get_folder().get_initial_editing_layer();
}

bool DrawingProgramLayerManager::is_a_layer_being_edited() {
    return !editingLayer.expired();
}

uint32_t DrawingProgramLayerManager::edited_layer_component_count() {
    return editingLayer.lock()->get_layer().components->size();
}

uint32_t DrawingProgramLayerManager::total_component_count() {
    if(layerTreeRoot)
        return layerTreeRoot->get_component_count();
    return 0;
}

CanvasComponentContainer::ObjInfoIterator DrawingProgramLayerManager::get_edited_layer_end_iterator() {
    return editingLayer.lock()->get_layer().components->end();
}

void DrawingProgramLayerManager::draw(SkCanvas* canvas, const DrawData& drawData) {
    if(layerTreeRoot)
        layerTreeRoot->draw(canvas, drawData);
}

std::vector<CanvasComponentContainer::ObjInfo*> DrawingProgramLayerManager::get_flattened_component_list() const {
    std::vector<CanvasComponentContainer::ObjInfo*> toRet;
    layerTreeRoot->get_flattened_component_list(toRet);
    return toRet;
}

std::vector<DrawingProgramLayerListItem*> DrawingProgramLayerManager::get_flattened_layer_list() {
    std::vector<DrawingProgramLayerListItem*> toRet;
    layerTreeRoot->get_flattened_layer_list(toRet);
    return toRet;
}

bool DrawingProgramLayerManager::layer_tree_root_exists() {
    return layerTreeRoot;
}

const DrawingProgramLayerListItem& DrawingProgramLayerManager::get_layer_root() {
    return *layerTreeRoot;
}

bool DrawingProgramLayerManager::component_passes_layer_selector(CanvasComponentContainer::ObjInfo* c, LayerSelector layerSelector) {
    switch(layerSelector) {
        case LayerSelector::ALL_VISIBLE_LAYERS:
            return c->obj->parentLayer->get_visible();
        case LayerSelector::LAYER_BEING_EDITED:
            return is_a_layer_being_edited() && c->obj->parentLayer == editingLayer.lock().get();
    }
    return false;
}

std::vector<CanvasComponentContainer::ObjInfoIterator> DrawingProgramLayerManager::add_many_components_to_layer_being_edited(const std::vector<std::pair<CanvasComponentContainer::ObjInfoIterator, CanvasComponentContainer*>>& newObjs) {
    if(!newObjs.empty()) {
        auto editLayerPtr = editingLayer.lock();
        if(!editLayerPtr)
            throw std::runtime_error("[DrawingProgramLayerManager::add_many_components_to_layer_being_edited] No layer selected!");
        auto toRet = editLayerPtr->get_layer().components->insert_ordered_list_and_send_create(editLayerPtr->get_layer().components, newObjs);
        add_undo_place_components(editingLayer.lock().get(), toRet);
    }
    return {};
}

CanvasComponentContainer::ObjInfo* DrawingProgramLayerManager::add_component_to_layer_being_edited(CanvasComponentContainer* newObj) {
    auto editLayerPtr = editingLayer.lock();
    if(!editLayerPtr)
        throw std::runtime_error("[DrawingProgramLayerManager::add_component_to_layer_being_edited] No layer selected!");
    return &(*editLayerPtr->get_layer().components->push_back_and_send_create(editLayerPtr->get_layer().components, newObj));
}

void DrawingProgramLayerManager::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    if(version >= VersionNumber(0, 4, 0)) {
        layerTreeRoot = drawP.world.netObjMan.make_obj_from_ptr<DrawingProgramLayerListItem>(new DrawingProgramLayerListItem());
        layerTreeRoot->load_file(a, version, *this);
        commitUpdateOnComponentInsert = false;
        layerTreeRoot->get_folder().set_component_list_callbacks(*this); // Only need to call set_component_list_callbacks on root, and the rest will get the callbacks set as well
        commitUpdateOnComponentInsert = true;
    }
    else {
        server_init_no_file();
        commitUpdateOnComponentInsert = false;
        CanvasComponentContainer::NetListTemporaryPtr editingLayerTmpPtrComponents = editingLayer.lock()->get_layer().components;
        uint64_t compCount;
        a(compCount);
        for(uint64_t i = 0; i < compCount; i++) {
            CanvasComponentContainer* newContainer = new CanvasComponentContainer();
            newContainer->load_file(a, version, drawP.world.netObjMan);
            editingLayerTmpPtrComponents->push_back_and_send_create(editingLayerTmpPtrComponents, newContainer);
        }
        commitUpdateOnComponentInsert = true;
    }
    auto flattenedCompList = get_flattened_component_list();
    parallel_loop_container(flattenedCompList, [&drawP = drawP](CanvasComponentContainer::ObjInfo* comp) {
        comp->obj->commit_update_dont_invalidate_cache(drawP);
    });
    drawP.rebuild_cache();
    editingLayer = layerTreeRoot->get_folder().get_initial_editing_layer();
}

void DrawingProgramLayerManager::save_file(cereal::PortableBinaryOutputArchive& a) const {
    layerTreeRoot->save_file(a);
}

void DrawingProgramLayerManager::get_used_resources(std::unordered_set<NetworkingObjects::NetObjID>& resourceSet) {
    layerTreeRoot->get_used_resources(resourceSet);
}

void DrawingProgramLayerManager::add_undo_place_component(CanvasComponentContainer::ObjInfo* objInfo) {
    using namespace NetworkingObjects;
    class AddCanvasComponentWorldUndoAction : public WorldUndoAction {
        public:
            AddCanvasComponentWorldUndoAction(uint32_t newPos, WorldUndoManager::UndoObjectID initUndoID, WorldUndoManager::UndoObjectID initComponentListUndoID):
                pos(newPos),
                undoID(initUndoID),
                componentListUndoID(initComponentListUndoID)
            {}
            std::string get_name() const override {
                return "Add Canvas Object";
            }
            bool undo(WorldUndoManager& undoMan) override {
                std::optional<NetObjID> toEraseID = undoMan.get_netid_from_undoid(undoID);
                if(!toEraseID.has_value())
                    return false;
                std::optional<NetObjID> toEraseCompListID = undoMan.get_netid_from_undoid(componentListUndoID);
                if(!toEraseCompListID.has_value())
                    return false;
                auto compList = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<NetObjOrderedList<CanvasComponentContainer>>(toEraseCompListID.value());
                auto toEraseIt = compList->get(toEraseID.value());
                copyData = toEraseIt->obj->get_data_copy();
                compList->erase(compList, toEraseIt);
                return true;
            }
            bool redo(WorldUndoManager& undoMan) override {
                std::optional<NetworkingObjects::NetObjID> toInsertID = undoMan.get_netid_from_undoid(undoID);
                if(toInsertID.has_value())
                    return false;
                std::optional<NetObjID> toEraseCompListID = undoMan.get_netid_from_undoid(componentListUndoID);
                if(!toEraseCompListID.has_value())
                    return false;
                auto compList = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<NetObjOrderedList<CanvasComponentContainer>>(toEraseCompListID.value());
                auto insertedIt = compList->emplace_direct(compList, compList->at(pos), undoMan.world.netObjMan, *copyData);
                undoMan.register_new_netid_to_existing_undoid(undoID, insertedIt->obj.get_net_id());
                copyData = nullptr;
                return true;
            }
            void scale_up(const WorldScalar& scaleAmount) override {
                if(copyData)
                    copyData->scale_up(scaleAmount);
            }
            ~AddCanvasComponentWorldUndoAction() {}

            std::unique_ptr<CanvasComponentContainer::CopyData> copyData;
            uint32_t pos;
            WorldUndoManager::UndoObjectID undoID;
            WorldUndoManager::UndoObjectID componentListUndoID;
    };

    drawP.world.undo.push(std::make_unique<AddCanvasComponentWorldUndoAction>(objInfo->pos, drawP.world.undo.get_undoid_from_netid(objInfo->obj.get_net_id()), drawP.world.undo.get_undoid_from_netid(objInfo->obj->parentLayer->get_layer().components.get_net_id())));
}

void DrawingProgramLayerManager::erase_component_map(const std::unordered_map<DrawingProgramLayerListItem*, std::vector<CanvasComponentContainer::ObjInfoIterator>>& eraseMap) {
    add_undo_erase_components(eraseMap);
    drawP.world.netObjMan.send_multi_update_messsage([&]() {
        for(auto& [layerListItem, netObjSetToErase] : eraseMap) {
            auto& layerComponentList = layerListItem->get_layer().components;
            layerComponentList->erase_list(layerComponentList, netObjSetToErase);
        }
    }, NetworkingObjects::NetObjManager::SendUpdateType::SEND_TO_ALL, nullptr);
}

void DrawingProgramLayerManager::add_undo_erase_components(const std::unordered_map<DrawingProgramLayerListItem*, std::vector<CanvasComponentContainer::ObjInfoIterator>>& eraseMap) {
    using namespace NetworkingObjects;

    struct EraseListData {
        std::vector<WorldUndoManager::UndoObjectID> undoID;
        std::vector<uint32_t> pos;
        std::vector<std::unique_ptr<CanvasComponentContainer::CopyData>> copyData;
    };

    class EraseCanvasComponentsWorldUndoAction : public WorldUndoAction {
        public:
            EraseCanvasComponentsWorldUndoAction(std::unordered_map<WorldUndoManager::UndoObjectID, EraseListData> initEraseMap):
                undoEraseMap(std::move(initEraseMap))
            {}
            std::string get_name() const override {
                return "Erase Canvas Objects";
            }
            bool undo(WorldUndoManager& undoMan) override {
                struct InsertData {
                    std::vector<NetObjOrderedListIterator<CanvasComponentContainer>> it;
                    std::vector<std::unique_ptr<CanvasComponentContainer::CopyData>>* copyData;
                    std::vector<WorldUndoManager::UndoObjectID>* undoID;
                };

                std::unordered_map<NetObjID, InsertData> insertMap;

                {
                    for(auto& [parentUndoID, eData] : undoEraseMap) {
                        std::optional<NetObjID> parentNetID = undoMan.get_netid_from_undoid(parentUndoID);
                        if(!parentNetID)
                            return false;
                        auto& insertData = insertMap[parentNetID.value()];
                        auto parentListPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<NetObjOrderedList<CanvasComponentContainer>>(parentNetID.value());
                        insertData.it = parentListPtr->at_ordered_indices(eData.pos);
                        insertData.copyData = &eData.copyData;
                        insertData.undoID = &eData.undoID;
                    }
                }

                undoMan.world.netObjMan.send_multi_update_messsage([&]() {
                    for(auto& [parentNetID, insertData] : insertMap) {
                        auto parentListPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<NetObjOrderedList<CanvasComponentContainer>>(parentNetID);
                        std::vector<std::pair<NetObjOrderedListIterator<CanvasComponentContainer>, NetObjOwnerPtr<CanvasComponentContainer>>> toInsert(insertData.it.size());
                        for(size_t i = 0; i < insertData.it.size(); i++) {
                            toInsert[i].first = insertData.it[i];
                            toInsert[i].second = undoMan.world.netObjMan.make_obj_direct<CanvasComponentContainer>(undoMan.world.netObjMan, *insertData.copyData->at(i));
                            undoMan.register_new_netid_to_existing_undoid(insertData.undoID->at(i), toInsert[i].second.get_net_id());
                        }
                        parentListPtr->insert_ordered_list_and_send_create(parentListPtr, toInsert);
                        insertData.copyData->clear();
                    }
                }, NetworkingObjects::NetObjManager::SendUpdateType::SEND_TO_ALL, nullptr);

                return true;
            }
            bool redo(WorldUndoManager& undoMan) override {
                std::unordered_map<NetObjID, std::vector<NetObjOrderedListIterator<CanvasComponentContainer>>> eraseMap;

                {
                    for(auto& [parentUndoID, eData] : undoEraseMap) {
                        std::optional<NetObjID> parentNetID = undoMan.get_netid_from_undoid(parentUndoID);
                        if(!parentNetID)
                            return false;
                        auto& eraseData = eraseMap[parentNetID.value()];
                        auto parentListPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<NetObjOrderedList<CanvasComponentContainer>>(parentNetID.value());
                        std::vector<NetObjID> netIDList;
                        if(!undoMan.fill_netid_list_from_undoid_list(netIDList, eData.undoID))
                            return false;
                        eraseData = parentListPtr->get_list(netIDList);
                        for(auto& it : eraseData)
                            eData.copyData.emplace_back(it->obj->get_data_copy());
                    }
                }

                undoMan.world.netObjMan.send_multi_update_messsage([&]() {
                    for(auto& [parentNetID, eData] : eraseMap) {
                        auto parentListPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<NetObjOrderedList<CanvasComponentContainer>>(parentNetID);
                        parentListPtr->erase_list(parentListPtr, eData);
                    }
                }, NetworkingObjects::NetObjManager::SendUpdateType::SEND_TO_ALL, nullptr);

                return true;
            }
            void scale_up(const WorldScalar& scaleAmount) override {
                for(auto& [undoID, eData] : undoEraseMap) {
                    for(auto& c : eData.copyData)
                        c->scale_up(scaleAmount);
                }
            }
            ~EraseCanvasComponentsWorldUndoAction() {}

            std::unordered_map<WorldUndoManager::UndoObjectID, EraseListData> undoEraseMap;
    };

    auto& undoMan = drawP.world.undo;
    std::unordered_map<WorldUndoManager::UndoObjectID, EraseListData> undoMap;
    for(auto& [item, compContainer] : eraseMap) {
        auto& undoData = undoMap[undoMan.get_undoid_from_netid(item->get_layer().components.get_net_id())];
        // Vectors must be sorted
        uint32_t i = 0;
        for(auto& comp : compContainer) {
            undoData.undoID.emplace_back(undoMan.get_undoid_from_netid(comp->obj.get_net_id()));
            undoData.copyData.emplace_back(comp->obj->get_data_copy());
            undoData.pos.emplace_back(comp->pos - i);
            ++i;
        }
    }
    undoMan.push(std::make_unique<EraseCanvasComponentsWorldUndoAction>(std::move(undoMap)));
}

void DrawingProgramLayerManager::add_undo_place_components(DrawingProgramLayerListItem* parent, const std::vector<CanvasComponentContainer::ObjInfoIterator>& placeList) {
    using namespace NetworkingObjects;

    struct PlaceListData {
        std::vector<WorldUndoManager::UndoObjectID> undoID;
        std::vector<uint32_t> pos;
        std::vector<std::unique_ptr<CanvasComponentContainer::CopyData>> copyData;
    };

    class PlaceCanvasComponentsWorldUndoAction : public WorldUndoAction {
        public:
            PlaceCanvasComponentsWorldUndoAction(WorldUndoManager::UndoObjectID parentList, PlaceListData initPlaceData):
                parentUndoID(parentList),
                undoPlaceData(std::move(initPlaceData))
            {}
            std::string get_name() const override {
                return "Place Canvas Objects";
            }
            bool undo(WorldUndoManager& undoMan) override {
                NetObjTemporaryPtr<NetObjOrderedList<CanvasComponentContainer>> parentListPtr;
                std::vector<NetObjOrderedListIterator<CanvasComponentContainer>> eraseData;

                {
                        std::optional<NetObjID> parentNetID = undoMan.get_netid_from_undoid(parentUndoID);
                        if(!parentNetID)
                            return false;
                        parentListPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<NetObjOrderedList<CanvasComponentContainer>>(parentNetID.value());
                }

                {
                    std::vector<NetObjID> netIDList;
                    if(!undoMan.fill_netid_list_from_undoid_list(netIDList, undoPlaceData.undoID))
                        return false;
                    eraseData = parentListPtr->get_list(netIDList);
                    for(auto& it : eraseData)
                        undoPlaceData.copyData.emplace_back(it->obj->get_data_copy());
                }

                parentListPtr->erase_list(parentListPtr, eraseData);

                return true;
            }
            bool redo(WorldUndoManager& undoMan) override {
                std::vector<NetObjOrderedListIterator<CanvasComponentContainer>> it;
                NetObjTemporaryPtr<NetObjOrderedList<CanvasComponentContainer>> parentListPtr;

                {
                    std::optional<NetObjID> parentNetID = undoMan.get_netid_from_undoid(parentUndoID);
                    if(!parentNetID)
                        return false;
                    parentListPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<NetObjOrderedList<CanvasComponentContainer>>(parentNetID.value());
                }

                {
                    it = parentListPtr->at_ordered_indices(undoPlaceData.pos);
                }

                std::vector<std::pair<NetObjOrderedListIterator<CanvasComponentContainer>, NetObjOwnerPtr<CanvasComponentContainer>>> toInsert(it.size());
                for(size_t i = 0; i < it.size(); i++) {
                    toInsert[i].first = it[i];
                    toInsert[i].second = undoMan.world.netObjMan.make_obj_direct<CanvasComponentContainer>(undoMan.world.netObjMan, *undoPlaceData.copyData.at(i));
                    undoMan.register_new_netid_to_existing_undoid(undoPlaceData.undoID.at(i), toInsert[i].second.get_net_id());
                }
                parentListPtr->insert_ordered_list_and_send_create(parentListPtr, toInsert);

                undoPlaceData.copyData.clear();
                return true;
            }
            void scale_up(const WorldScalar& scaleAmount) override {
                for(auto& c : undoPlaceData.copyData)
                    c->scale_up(scaleAmount);
            }
            ~PlaceCanvasComponentsWorldUndoAction() {}

            WorldUndoManager::UndoObjectID parentUndoID;
            PlaceListData undoPlaceData;
    };

    auto& undoMan = drawP.world.undo;
    PlaceListData undoData;
    // Vectors must be sorted
    uint32_t i = 0;
    for(auto& comp : placeList) {
        undoData.undoID.emplace_back(undoMan.get_undoid_from_netid(comp->obj.get_net_id()));
        undoData.pos.emplace_back(comp->pos - i);
        ++i;
    }
    undoMan.push(std::make_unique<PlaceCanvasComponentsWorldUndoAction>(undoMan.get_undoid_from_netid(parent->get_layer().components.get_net_id()), std::move(undoData)));
}
