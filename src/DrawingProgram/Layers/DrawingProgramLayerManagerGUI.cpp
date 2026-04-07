#include "DrawingProgramLayerManagerGUI.hpp"
#include "DrawingProgramLayerListItem.hpp"
#include "DrawingProgramLayerManager.hpp"
#include "../DrawingProgram.hpp"
#include "../../World.hpp"
#include "../../MainProgram.hpp"
#include "Helpers/ConvertVec.hpp"
#include "Helpers/NetworkingObjects/NetObjOrderedList.hpp"
#include "SerializedBlendMode.hpp"

#include "../../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/ButtonHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/LayoutHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/TextBoxHelpers.hpp"
#include "../../GUIStuff/ElementHelpers/NumberSliderHelpers.hpp"
#include "../../GUIStuff/Elements/SVGIcon.hpp"
#include "../../GUIStuff/Elements/DropDown.hpp"

DrawingProgramLayerManagerGUI::DrawingProgramLayerManagerGUI(DrawingProgramLayerManager& drawLayerMan):
    layerMan(drawLayerMan)
{}

void DrawingProgramLayerManagerGUI::refresh_gui_data() {
    selectedLayerIndices.clear();
    editing_layer_check();
    nameToEdit.clear();
    nameForNew.clear();
    editingLayerOldMetainfo = std::nullopt;
    alphaValToEdit = 0.0f;
    blendModeValToEdit = 0;
}

NetworkingObjects::NetObjTemporaryPtr<DrawingProgramLayerListItem> DrawingProgramLayerManagerGUI::get_layer_parent_from_obj_index(const GUIStuff::TreeListingObjIndexList& objIndex) {
    if(objIndex.empty())
        throw std::runtime_error("[DrawingProgramLayerManagerGUI::get_layer_parent_from_obj_index] Called on empty objIndex");
    NetworkingObjects::NetObjTemporaryPtr<DrawingProgramLayerListItem> toRet = layerMan.layerTreeRoot;
    for(size_t i = 0; i < objIndex.size() - 1; i++)
        toRet = toRet->get_folder().folderList->at(objIndex[i])->obj;
    return toRet;
}

NetworkingObjects::NetObjTemporaryPtr<DrawingProgramLayerListItem> DrawingProgramLayerManagerGUI::get_layer_from_obj_index(const GUIStuff::TreeListingObjIndexList& objIndex) {
    NetworkingObjects::NetObjTemporaryPtr<DrawingProgramLayerListItem> toRet = layerMan.layerTreeRoot;
    for(size_t i : objIndex)
        toRet = toRet->get_folder().folderList->at(i)->obj;
    return toRet;
}

void DrawingProgramLayerManagerGUI::setup_list_gui() {
    using namespace NetworkingObjects;
    using namespace GUIStuff;
    using namespace ElementHelpers;

    auto& world = layerMan.drawP.world;
    if(layerMan.layerTreeRoot) {
        auto& gui = world.main.g.gui;
        if(layerMan.layerTreeRoot->get_folder().folderList->empty()) {
            text_label_centered(gui, "No layers exist.");
            selectedLayerIndices.clear();
            nameToEdit.clear();
        }
        else {
            gui.element<TreeListing>("list", TreeListing::Data {
                .selectedIndices = &selectedLayerIndices,
                .dirInfo = [&](const TreeListingObjIndexList& objIndex) -> std::optional<TreeListing::DirectoryInfo> {
                    auto layer = get_layer_from_obj_index(objIndex);
                    if(!layer->is_folder())
                        return std::nullopt;
                    else
                        return TreeListing::DirectoryInfo{
                            .dirSize = layer->get_folder().folderList->size(),
                            .isOpen = layer->get_folder().isFolderOpen
                        };
                },
                .setDirectoryOpen = [&](const TreeListingObjIndexList& objIndex, bool newDirectoryOpen) {
                    get_layer_from_obj_index(objIndex)->get_folder().isFolderOpen = newDirectoryOpen;
                },
                .drawNonDirectoryObjIconGUI = [&](const TreeListingObjIndexList& objIndex) {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            .padding = CLAY_PADDING_ALL(2),
                        },
                    }) {
                        gui.element<SVGIcon>("layer ico", "data/icons/layer.svg");
                    }
                },
                .drawObjGUI = [&](const TreeListingObjIndexList& objIndex) {
                    auto layer = get_layer_from_obj_index(objIndex);
                    if(!layer->is_folder()) {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_FIXED(GUIStuff::TreeListing::ENTRY_HEIGHT), .height = CLAY_SIZING_FIXED(GUIStuff::TreeListing::ENTRY_HEIGHT)},
                                .padding = CLAY_PADDING_ALL(2),
                            },
                        }) {
                            if(layer.get_net_id() == layerMan.editingLayer.get_net_id())
                                gui.element<SVGIcon>("edit ico", "data/icons/pencil.svg");
                        }
                    }
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                        },
                    }) {
                        text_label(gui, layer->get_name());
                    }
                    if(!layer->is_folder()) {
                        gui.set_z_index_keep_clipping_region(gui.get_z_index() + 1, [&] {
                            svg_icon_button(gui, "edit button", "data/icons/pencil.svg", {
                                .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                                .size = TreeListing::ENTRY_HEIGHT,
                                .onClick = [&, layer] {
                                    layerMan.editingLayer = layer;
                                }
                            });
                        });
                    }
                    gui.set_z_index_keep_clipping_region(gui.get_z_index() + 1, [&] {
                        svg_icon_button(gui, "visible button", layer->get_visible() ? "data/icons/eyeopen.svg" : "data/icons/eyeclose.svg", {
                            .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                            .size = TreeListing::ENTRY_HEIGHT,
                            .onClick = [&, layer] {
                                layer->set_visible(layerMan, !layer->get_visible());
                            }
                        });
                    });
                    gui.set_z_index_keep_clipping_region(gui.get_z_index() + 1, [&] {
                        svg_icon_button(gui, "delete button", "data/icons/trash.svg", {
                            .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                            .size = TreeListing::ENTRY_HEIGHT,
                            .onClick = [&, objIndex] {
                                remove_layer(objIndex);
                            }
                        });
                    });
                },
                .onDoubleClick = [&](const TreeListingObjIndexList& objIndex) {
                    layerMan.editingLayer = get_layer_from_obj_index(objIndex);
                    layerMan.drawP.world.main.g.gui.set_to_layout();
                },
                .onSelectChange = [&] {
                    editing_layer_check();
                    if(selectedLayerIndices.size() == 1) {
                        const TreeListingObjIndexList& objIndex = *selectedLayerIndices.begin();
                        NetObjTemporaryPtr<DrawingProgramLayerListItem> tempPtr = get_layer_from_obj_index(objIndex);
                        if(tempPtr) {
                            nameToEdit = tempPtr->get_name();
                            alphaValToEdit = tempPtr->get_alpha();
                            auto it = std::find(get_blend_mode_useful_list().begin(), get_blend_mode_useful_list().end(), tempPtr->get_blend_mode());
                            blendModeValToEdit = (it == get_blend_mode_useful_list().end()) ? 0 : (it - get_blend_mode_useful_list().begin());
                        }
                        else {
                            nameToEdit.clear();
                            alphaValToEdit = 0.0f;
                            blendModeValToEdit = 0;
                        }
                    }
                    else {
                        nameToEdit.clear();
                        alphaValToEdit = 0.0f;
                        blendModeValToEdit = 0;
                    }
                },
                .moveObj = [&](const std::vector<TreeListingObjIndexList>& objectIndicesTreeListing, const TreeListingObjIndexList& newObjIndexTreeListing) {
                    NetObjID listObj;
                    size_t index;
                    struct ParentObjectIDPair {
                        NetObjID parent;
                        NetObjID object;
                    };
                    std::vector<ParentObjectIDPair> objsToInsert;

                    {
                        listObj = get_layer_parent_from_obj_index(newObjIndexTreeListing).get_net_id();
                        index = newObjIndexTreeListing.back();
                        for(auto& objIndex : objectIndicesTreeListing)
                            objsToInsert.emplace_back(get_layer_parent_from_obj_index(objIndex).get_net_id(), get_layer_from_obj_index(objIndex).get_net_id());
                    }

                    std::unordered_map<WorldUndoManager::UndoObjectID, std::pair<std::vector<uint32_t>, std::vector<WorldUndoManager::UndoObjectID>>> toEraseMapUndo;
                    WorldUndoManager::UndoObjectID insertParentUndoID;
                    uint32_t insertUndoPosition;
                    std::vector<WorldUndoManager::UndoObjectID> insertedUndoIDs;

                    world.netObjMan.send_multi_update_messsage([&]() {
                        std::unordered_map<NetObjID, std::vector<NetObjOrderedListIterator<DrawingProgramLayerListItem>>> toEraseMap;
                        uint32_t newIndex = index;
                        std::vector<NetObjOwnerPtr<DrawingProgramLayerListItem>> toInsertObjPtrs;
                        auto& listPtr = world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(listObj)->get_folder().folderList;

                        // Map parent folders to list of objects that should be erased from them
                        for(size_t i = 0; i < objsToInsert.size(); i++) {
                            auto& parentListPtr = world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(objsToInsert[i].parent)->get_folder().folderList;
                            toEraseMap[objsToInsert[i].parent].emplace_back(parentListPtr->get(objsToInsert[i].object));
                            if(newIndex != 0 && objsToInsert[i].parent == listObj && listPtr->get(objsToInsert[i].object)->pos <= index)
                                newIndex--;
                        }

                        // Prepare erase map for undo
                        for(auto& [parentNetID, objsToEraseList] : toEraseMap) {
                            auto& undoEraseListPairForParent = toEraseMapUndo[world.undo.get_undoid_from_netid(parentNetID)];
                            uint32_t i = 0;
                            for(auto& objToErase : objsToEraseList) {
                                undoEraseListPairForParent.first.emplace_back(objToErase->pos - i);
                                undoEraseListPairForParent.second.emplace_back(world.undo.get_undoid_from_netid(objToErase->obj.get_net_id()));
                                i++;
                            }
                        }

                        // Erase the list of objects we previously mapped to each parent, and move the erased objects to vector toInsertObjPtrs
                        for(auto& [listToEraseFrom, setToErase] : toEraseMap) {
                            auto& listToEraseFromPtr = world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(listToEraseFrom)->get_folder().folderList;
                            listToEraseFromPtr->erase_list(listToEraseFromPtr, setToErase, &toInsertObjPtrs);
                        }

                        // Insert objects in toInsertObjPtrs into the folder we have selected
                        std::vector<std::pair<NetObjOrderedListIterator<DrawingProgramLayerListItem>, NetObjOwnerPtr<DrawingProgramLayerListItem>>> toInsert;
                        auto insertIt = listPtr->at(newIndex);
                        for(uint32_t i = 0; i < objsToInsert.size(); i++) {
                            auto it = std::find_if(toInsertObjPtrs.begin(), toInsertObjPtrs.end(), [id = objsToInsert[i].object](NetObjOwnerPtr<DrawingProgramLayerListItem>& objPtr) {
                                return objPtr.get_net_id() == id;
                            });
                            toInsert.emplace_back(insertIt, std::move(*it));
                            toInsert.back().second.reassign_ids();
                        }
                        std::vector<NetObjOrderedListIterator<DrawingProgramLayerListItem>> insertedIterators = listPtr->insert_sorted_list_and_send_create(listPtr, toInsert);

                        // Prepare insert list for undo
                        insertParentUndoID = world.undo.get_undoid_from_netid(listObj);
                        insertUndoPosition = newIndex;
                        for(auto& it : insertedIterators)
                            insertedUndoIDs.emplace_back(world.undo.get_undoid_from_netid(it->obj.get_net_id()));
                    }, NetObjManager::SendUpdateType::SEND_TO_ALL, nullptr);

                    class MoveLayersWorldUndoAction : public WorldUndoAction {
                        public:
                            // If the undo ID maps to the net id, we can also assume that the object is definitely inside the list we want to move it from, and not inside some other list
                            // If it was moved to another list, there would be an undo move that would move it back to this list, and if it was moved by another player the undo id wouldnt map back to the net id, and the undo would fail
                            // We can also assume all objects in all lists remain in the same order for the same reason, so no need to sort anything if it was sorted when the undo action was generated
                            MoveLayersWorldUndoAction(const std::unordered_map<WorldUndoManager::UndoObjectID, std::pair<std::vector<uint32_t>, std::vector<WorldUndoManager::UndoObjectID>>>& initUndoEraseMap,
                                                         WorldUndoManager::UndoObjectID initInsertParentUndoID,
                                                         uint32_t initInsertPosition,
                                                         std::vector<WorldUndoManager::UndoObjectID> initInsertedUndoIDs):
                            undoEraseMap(initUndoEraseMap),
                            insertParentUndoID(initInsertParentUndoID),
                            insertPosition(initInsertPosition),
                            insertedUndoIDs(initInsertedUndoIDs)
                            {}
                            std::string get_name() const override {
                                return "Move Layers";
                            }
                            bool undo(WorldUndoManager& undoMan) override {
                                std::vector<NetObjOwnerPtr<DrawingProgramLayerListItem>> toMoveObjs;
                                std::vector<NetObjOrderedListIterator<DrawingProgramLayerListItem>> toEraseList;
                                NetObjTemporaryPtr<NetObjOrderedList<DrawingProgramLayerListItem>> eraseListPtr;
                                std::unordered_map<NetObjID, std::pair<std::vector<uint32_t>*, std::vector<NetObjID>>> toInsertToMap;

                                {
                                    std::optional<NetObjID> eraseListNetID = undoMan.get_netid_from_undoid(insertParentUndoID);
                                    if(!eraseListNetID.has_value())
                                        return false;
                                    eraseListPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(eraseListNetID.value())->get_folder().folderList;
                                }

                                {
                                    std::vector<NetObjID> toEraseNetIDList;
                                    if(!undoMan.fill_netid_list_from_undoid_list(toEraseNetIDList, insertedUndoIDs))
                                        return false;
                                    toEraseList = eraseListPtr->get_list(toEraseNetIDList);
                                }

                                {
                                    for(auto& [undoID, indexUndoIdListPair] : undoEraseMap) {
                                        std::optional<NetObjID> netID = undoMan.get_netid_from_undoid(undoID);
                                        if(!netID.has_value())
                                            return false;
                                        auto& insertMapEntry = toInsertToMap[netID.value()];
                                        insertMapEntry.first = &indexUndoIdListPair.first;
                                        if(!undoMan.fill_netid_list_from_undoid_list(insertMapEntry.second, indexUndoIdListPair.second))
                                            return false;
                                    }
                                }

                                undoMan.world.netObjMan.send_multi_update_messsage([&]() {
                                    eraseListPtr->erase_list(eraseListPtr, toEraseList, &toMoveObjs);

                                    for(auto& [parentNetID, indexNetIDListPair] : toInsertToMap) {
                                        auto& parentListPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(parentNetID)->get_folder().folderList;
                                        std::vector<NetObjOrderedListIterator<DrawingProgramLayerListItem>> insertIteratorList = parentListPtr->at_ordered_indices(*indexNetIDListPair.first);
                                        auto& insertNetIDList = indexNetIDListPair.second;
                                        std::vector<std::pair<NetObjOrderedListIterator<DrawingProgramLayerListItem>, NetObjOwnerPtr<DrawingProgramLayerListItem>>> toInsert;
                                        for(auto [insertIt, netID] : std::views::zip(insertIteratorList, insertNetIDList)) {
                                            auto it = std::find_if(toMoveObjs.begin(), toMoveObjs.end(), [id = netID](NetObjOwnerPtr<DrawingProgramLayerListItem>& objPtr) {
                                                return objPtr.get_net_id() == id;
                                            });
                                            toInsert.emplace_back(insertIt, std::move(*it));
                                            toInsert.back().second.reassign_ids();
                                        }
                                        parentListPtr->insert_sorted_list_and_send_create(parentListPtr, toInsert);
                                    }
                                }, NetObjManager::SendUpdateType::SEND_TO_ALL, nullptr);
                                return true;
                            }
                            bool redo(WorldUndoManager& undoMan) override {
                                std::unordered_map<NetObjID, std::vector<NetObjID>> toEraseMap;
                                NetObjTemporaryPtr<NetObjOrderedList<DrawingProgramLayerListItem>> listPtr;
                                std::vector<NetObjID> objsToInsert;

                                {
                                    for(auto& [undoID, indexUndoIDListPair] : undoEraseMap) {
                                        std::optional<NetObjID> netID = undoMan.get_netid_from_undoid(undoID);
                                        if(!netID.has_value())
                                            return false;
                                        if(!undoMan.fill_netid_list_from_undoid_list(toEraseMap[netID.value()], indexUndoIDListPair.second))
                                            return false;
                                    }
                                }

                                {
                                    std::optional<NetObjID> netID = undoMan.get_netid_from_undoid(insertParentUndoID);
                                    if(!netID.has_value())
                                        return false;
                                    listPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(netID.value())->get_folder().folderList;
                                }

                                {
                                    if(!undoMan.fill_netid_list_from_undoid_list(objsToInsert, insertedUndoIDs))
                                        return false;
                                }

                                undoMan.world.netObjMan.send_multi_update_messsage([&]() {
                                    std::vector<NetObjOwnerPtr<DrawingProgramLayerListItem>> toInsertObjPtrs;

                                    for(auto& [listToEraseFrom, setToErase] : toEraseMap) {
                                        auto& listToEraseFromPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(listToEraseFrom)->get_folder().folderList;
                                        listToEraseFromPtr->erase_list(listToEraseFromPtr, listToEraseFromPtr->get_list(setToErase), &toInsertObjPtrs);
                                    }

                                    std::vector<std::pair<NetObjOrderedListIterator<DrawingProgramLayerListItem>, NetObjOwnerPtr<DrawingProgramLayerListItem>>> toInsert;
                                    auto insertIt = listPtr->at(insertPosition);
                                    for(uint32_t i = 0; i < objsToInsert.size(); i++) {
                                        auto it = std::find_if(toInsertObjPtrs.begin(), toInsertObjPtrs.end(), [id = objsToInsert[i]](NetObjOwnerPtr<DrawingProgramLayerListItem>& objPtr) {
                                            return objPtr.get_net_id() == id;
                                        });
                                        toInsert.emplace_back(insertIt, std::move(*it));
                                        toInsert.back().second.reassign_ids();
                                    }
                                    listPtr->insert_sorted_list_and_send_create(listPtr, toInsert);
                                }, NetObjManager::SendUpdateType::SEND_TO_ALL, nullptr);

                                return true;
                            }
                            ~MoveLayersWorldUndoAction() {}

                            std::unordered_map<WorldUndoManager::UndoObjectID, std::pair<std::vector<uint32_t>, std::vector<WorldUndoManager::UndoObjectID>>> undoEraseMap;
                            WorldUndoManager::UndoObjectID insertParentUndoID;
                            uint32_t insertPosition;
                            std::vector<WorldUndoManager::UndoObjectID> insertedUndoIDs;
                    };
                    world.undo.push(std::make_unique<MoveLayersWorldUndoAction>(toEraseMapUndo, insertParentUndoID, insertUndoPosition, insertedUndoIDs));
                }
            });
        }

        left_to_right_line_layout(gui, [&]() {
            input_text(gui, "input new name", &nameForNew, {
                .onEnter = [&] {
                    auto newLayerObjInfo = create_layer(new DrawingProgramLayerListItem(world.netObjMan, nameForNew, false));
                    layerMan.editingLayer = newLayerObjInfo->obj;
                    layerMan.drawP.world.main.g.gui.set_to_layout();
                }
            });

            svg_icon_button(gui, "create new layer", "data/icons/plusbold.svg", {
                .size = SMALL_BUTTON_SIZE,
                .onClick = [&] {
                    auto newLayerObjInfo = create_layer(new DrawingProgramLayerListItem(world.netObjMan, nameForNew, false));
                    layerMan.editingLayer = newLayerObjInfo->obj;
                    layerMan.drawP.world.main.g.gui.set_to_layout();
                }
            });

            svg_icon_button(gui, "create new folder", "data/icons/folderbold.svg", {
                .size = SMALL_BUTTON_SIZE,
                .onClick = [&] {
                    auto newFolderObjInfo = create_layer(new DrawingProgramLayerListItem(world.netObjMan, nameForNew, true));
                    newFolderObjInfo->obj->get_folder().isFolderOpen = true;
                }
            });
        });

        if(editingLayer) {
            text_label_centered(gui, editingLayer->is_folder() ? "Edit Layer Folder" : "Edit Layer");
            input_text_field(gui, "input edit name", "Name", &nameToEdit, {
                .onEdit = [&] {
                    if(editingLayer) { // This can be set to null on the way here (close layer popup when textbox is selected)
                        editingLayer->set_name(world.delayedUpdateObjectManager, nameToEdit);
                        world.main.g.gui.set_to_layout();
                    }
                }
            });
            slider_scalar_field(gui, "input alpha slider", "Alpha", &alphaValToEdit, 0.0f, 1.0f, {
                .decimalPrecision = 2,
                .onEdit = [&] {
                    if(editingLayer) {
                        editingLayer->set_name(world.delayedUpdateObjectManager, nameToEdit);
                        editingLayer->set_alpha(layerMan, alphaValToEdit);
                    }
                }
            });
            left_to_right_line_layout(gui, [&]() {
                text_label(gui, "Blend Mode");
                gui.element<DropDown<size_t>>("input blend mode", &blendModeValToEdit, get_blend_mode_useful_name_list(), DropdownOptions{
                    .width = 190.0f,
                    .onClick = [&] {
                        if(editingLayer)
                            editingLayer->set_blend_mode(layerMan, get_blend_mode_useful_list()[blendModeValToEdit]);
                    }
                });
            });
        }
    }
}

std::optional<std::pair<NetworkingObjects::NetObjID, NetworkingObjects::NetObjOrderedListIterator<DrawingProgramLayerListItem>>> DrawingProgramLayerManagerGUI::try_to_create_in_proper_position(DrawingProgramLayerListItem* newItem) {
    using namespace NetworkingObjects;
    if(selectedLayerIndices.empty())
        return std::nullopt;
    auto& lastObjSelected = *selectedLayerIndices.rbegin();
    NetObjTemporaryPtr<DrawingProgramLayerListItem> objectPtr = get_layer_from_obj_index(lastObjSelected);
    if(objectPtr->is_folder()) {
        objectPtr->get_folder().isFolderOpen = true;
        return std::pair<NetworkingObjects::NetObjID, NetworkingObjects::NetObjOrderedListIterator<DrawingProgramLayerListItem>>{objectPtr.get_net_id(), objectPtr->get_folder().folderList->insert_and_send_create(objectPtr->get_folder().folderList, objectPtr->get_folder().folderList->begin(), newItem)};
    }
    NetObjTemporaryPtr<DrawingProgramLayerListItem> parentPtr = get_layer_parent_from_obj_index(lastObjSelected);
    return std::pair<NetworkingObjects::NetObjID, NetworkingObjects::NetObjOrderedListIterator<DrawingProgramLayerListItem>>{parentPtr.get_net_id(), parentPtr->get_folder().folderList->insert_and_send_create(parentPtr->get_folder().folderList, parentPtr->get_folder().folderList->get(objectPtr.get_net_id()), newItem)};
}

std::pair<NetworkingObjects::NetObjID, NetworkingObjects::NetObjOrderedListIterator<DrawingProgramLayerListItem>> DrawingProgramLayerManagerGUI::create_in_proper_position(DrawingProgramLayerListItem* newItem) {
    auto toRet = try_to_create_in_proper_position(newItem);
    if(!toRet)
        return {layerMan.layerTreeRoot.get_net_id(), layerMan.layerTreeRoot->get_folder().folderList->insert_and_send_create(layerMan.layerTreeRoot->get_folder().folderList, layerMan.layerTreeRoot->get_folder().folderList->begin(), newItem)};
    return toRet.value();
}

NetworkingObjects::NetObjOrderedListIterator<DrawingProgramLayerListItem> DrawingProgramLayerManagerGUI::create_layer(DrawingProgramLayerListItem* newItem) {
    auto& world = layerMan.drawP.world;
    class AddLayerWorldUndoAction : public WorldUndoAction {
        public:
            AddLayerWorldUndoAction(std::unique_ptr<DrawingProgramLayerListItemUndoData> initLayerData, uint32_t newPos, WorldUndoManager::UndoObjectID initParentUndoID, WorldUndoManager::UndoObjectID initUndoID):
                layerData(std::move(initLayerData)),
                pos(newPos),
                parentUndoID(initParentUndoID),
                undoID(initUndoID)
            {}
            std::string get_name() const override {
                return "Add Layer Item";
            }
            bool undo(WorldUndoManager& undoMan) override {
                std::optional<NetworkingObjects::NetObjID> toEraseParentID = undoMan.get_netid_from_undoid(parentUndoID);
                if(!toEraseParentID.has_value())
                    return false;
                std::optional<NetworkingObjects::NetObjID> toEraseID = undoMan.get_netid_from_undoid(undoID);
                if(!toEraseID.has_value())
                    return false;

                auto& layerList = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(toEraseParentID.value())->get_folder().folderList;
                auto it = layerList->get(toEraseID.value());
                layerData = std::make_unique<DrawingProgramLayerListItemUndoData>(it->obj->get_undo_data(undoMan));
                layerList->erase(layerList, it);
                return true;
            }
            bool redo(WorldUndoManager& undoMan) override {
                std::optional<NetworkingObjects::NetObjID> toInsertParentID = undoMan.get_netid_from_undoid(parentUndoID);
                if(!toInsertParentID.has_value())
                    return false;
                std::optional<NetworkingObjects::NetObjID> toInsertID = undoMan.get_netid_from_undoid(undoID);
                if(toInsertID.has_value())
                    return false;

                auto& layerList = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(toInsertParentID.value())->get_folder().folderList;
                auto insertedIt = layerList->emplace_direct(layerList, layerList->at(pos), undoMan.world, *layerData);
                undoMan.register_new_netid_to_existing_undoid(undoID, insertedIt->obj.get_net_id());
                layerData = nullptr;
                return true;
            }
            void scale_up(const WorldScalar& scaleAmount) override {
                if(layerData)
                    layerData->scale_up(scaleAmount);
            }
            ~AddLayerWorldUndoAction() {}

            std::unique_ptr<DrawingProgramLayerListItemUndoData> layerData;
            uint32_t pos;
            WorldUndoManager::UndoObjectID parentUndoID;
            WorldUndoManager::UndoObjectID undoID;
    };

    auto insertedLayerPair = create_in_proper_position(newItem);
    auto& it = insertedLayerPair.second;
    world.undo.push(std::make_unique<AddLayerWorldUndoAction>(std::make_unique<DrawingProgramLayerListItemUndoData>(it->obj->get_undo_data(world.undo)), it->pos, world.undo.get_undoid_from_netid(insertedLayerPair.first), world.undo.get_undoid_from_netid(it->obj.get_net_id())));
    world.main.g.gui.set_to_layout();
    refresh_gui_data();
    return it;
}

void DrawingProgramLayerManagerGUI::remove_layer(const GUIStuff::TreeListingObjIndexList& objIndex) {
    refresh_gui_data();

    auto& world = layerMan.drawP.world;
    class DeleteLayerWorldUndoAction : public WorldUndoAction {
        public:
            DeleteLayerWorldUndoAction(std::unique_ptr<DrawingProgramLayerListItemUndoData> initLayerData, uint32_t newPos, WorldUndoManager::UndoObjectID initParentUndoID, WorldUndoManager::UndoObjectID initUndoID):
                layerData(std::move(initLayerData)),
                pos(newPos),
                parentUndoID(initParentUndoID),
                undoID(initUndoID)
            {}
            std::string get_name() const override {
                return "Delete Layer Item";
            }
            bool undo(WorldUndoManager& undoMan) override {
                std::optional<NetworkingObjects::NetObjID> toInsertParentID = undoMan.get_netid_from_undoid(parentUndoID);
                if(!toInsertParentID.has_value())
                    return false;
                std::optional<NetworkingObjects::NetObjID> toInsertID = undoMan.get_netid_from_undoid(undoID);
                if(toInsertID.has_value())
                    return false;

                auto& layerList = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(toInsertParentID.value())->get_folder().folderList;
                auto insertedIt = layerList->emplace_direct(layerList, layerList->at(pos), undoMan.world, *layerData);
                undoMan.register_new_netid_to_existing_undoid(undoID, insertedIt->obj.get_net_id());
                layerData = nullptr;
                return true;
            }
            bool redo(WorldUndoManager& undoMan) override {
                std::optional<NetworkingObjects::NetObjID> toEraseParentID = undoMan.get_netid_from_undoid(parentUndoID);
                if(!toEraseParentID.has_value())
                    return false;
                std::optional<NetworkingObjects::NetObjID> toEraseID = undoMan.get_netid_from_undoid(undoID);
                if(!toEraseID.has_value())
                    return false;

                auto& layerList = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(toEraseParentID.value())->get_folder().folderList;
                auto it = layerList->get(toEraseID.value());
                layerData = std::make_unique<DrawingProgramLayerListItemUndoData>(it->obj->get_undo_data(undoMan));
                layerList->erase(layerList, it);
                return true;
            }
            void scale_up(const WorldScalar& scaleAmount) override {
                if(layerData)
                    layerData->scale_up(scaleAmount);
            }
            ~DeleteLayerWorldUndoAction() {}

            std::unique_ptr<DrawingProgramLayerListItemUndoData> layerData;
            uint32_t pos;
            WorldUndoManager::UndoObjectID parentUndoID;
            WorldUndoManager::UndoObjectID undoID;
    };


    auto parentPtr = get_layer_parent_from_obj_index(objIndex);
    auto layerPtr = get_layer_from_obj_index(objIndex);
    auto it = parentPtr->get_folder().folderList->get(layerPtr.get_net_id());
    world.undo.push(std::make_unique<DeleteLayerWorldUndoAction>(std::make_unique<DrawingProgramLayerListItemUndoData>(layerPtr->get_undo_data(world.undo)), it->pos, world.undo.get_undoid_from_netid(parentPtr.get_net_id()), world.undo.get_undoid_from_netid(layerPtr.get_net_id())));
    parentPtr->get_folder().folderList->erase(parentPtr->get_folder().folderList, it);
    world.main.g.gui.set_to_layout();
}

void DrawingProgramLayerManagerGUI::editing_layer_check() {
    auto& world = layerMan.drawP.world;
    class EditLayerWorldUndoAction : public WorldUndoAction {
        public:
            EditLayerWorldUndoAction(std::unique_ptr<DrawingProgramLayerListItemMetaInfo> initData, WorldUndoManager::UndoObjectID initUndoID):
                data(std::move(initData)),
                undoID(initUndoID)
            {}
            std::string get_name() const override {
                return "Edit Layer";
            }
            bool undo(WorldUndoManager& undoMan) override {
                return undo_redo(undoMan);
            }
            bool redo(WorldUndoManager& undoMan) override {
                return undo_redo(undoMan);
            }
            bool undo_redo(WorldUndoManager& undoMan) {
                std::optional<NetworkingObjects::NetObjID> toModifyID = undoMan.get_netid_from_undoid(undoID);
                if(!toModifyID.has_value())
                    return false;
                auto objPtr = undoMan.world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(toModifyID.value());
                auto newData = std::make_unique<DrawingProgramLayerListItemMetaInfo>(objPtr->get_metainfo());
                objPtr->set_metainfo(undoMan.world.drawProg.layerMan, *data);
                data = std::move(newData);
                return true;
            }
            ~EditLayerWorldUndoAction() {}

            std::unique_ptr<DrawingProgramLayerListItemMetaInfo> data;
            WorldUndoManager::UndoObjectID undoID;
    };


    if(editingLayer) {
        const DrawingProgramLayerListItemMetaInfo& currentMetaData = editingLayer->get_metainfo();
        if(currentMetaData != editingLayerOldMetainfo.value())
            world.undo.push(std::make_unique<EditLayerWorldUndoAction>(std::make_unique<DrawingProgramLayerListItemMetaInfo>(editingLayerOldMetainfo.value()), world.undo.get_undoid_from_netid(editingLayer.get_net_id())));
    }

    if(selectedLayerIndices.size() == 1) {
        editingLayer = get_layer_from_obj_index(*selectedLayerIndices.begin());
        editingLayerOldMetainfo = editingLayer->get_metainfo();
    }
    else {
        editingLayer = NetworkingObjects::NetObjTemporaryPtr<DrawingProgramLayerListItem>();
        editingLayerOldMetainfo = std::nullopt;
    }
}
