#include "DrawingProgramLayerManagerGUI.hpp"
#include "DrawingProgramLayerManager.hpp"
#include "../DrawingProgram.hpp"
#include "../../World.hpp"
#include "../../MainProgram.hpp"
#include "Helpers/ConvertVec.hpp"
#include "SerializedBlendMode.hpp"

DrawingProgramLayerManagerGUI::DrawingProgramLayerManagerGUI(DrawingProgramLayerManager& drawLayerMan):
    layerMan(drawLayerMan)
{}

void DrawingProgramLayerManagerGUI::refresh_gui_data() {
    selectionData = GUIStuff::TreeListing::SelectionData();
    oldSelection.clear();
    nameToEdit.clear();
    nameForNew.clear();
    alphaValToEdit = 0.0f;
    blendModeValToEdit = 0;
}

void DrawingProgramLayerManagerGUI::setup_list_gui(const std::string& id, bool& hoveringOverDropdown) {
    using namespace NetworkingObjects;
    auto& world = layerMan.drawP.world;
    if(layerMan.layerTreeRoot) {
        auto& gui = world.main.toolbar.gui;
        gui.push_id(id);
        if(layerMan.layerTreeRoot->get_folder().folderList->empty()) {
            gui.text_label_centered("No layers exist.");
            selectionData = GUIStuff::TreeListing::SelectionData();
            oldSelection.clear();
            nameToEdit.clear();
        }
        else {
            std::optional<NetObjID> toDeleteParent;
            std::optional<NetObjID> toDeleteObject;
            gui.tree_listing("list", layerMan.layerTreeRoot.get_net_id(), GUIStuff::TreeListing::DisplayData{
                .getObjInListAtIndex = [&](NetObjID parentId, size_t index) -> std::optional<GUIStuff::TreeListing::DisplayData::ObjInList> {
                    NetObjOrderedList<DrawingProgramLayerListItem>& parentFolder = *world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(parentId)->get_folder().folderList;
                    if(parentFolder.size() <= index)
                        return std::nullopt;
                    return GUIStuff::TreeListing::DisplayData::ObjInList{
                        .id = parentFolder.at(index)->obj.get_net_id(),
                        .isDirectory = parentFolder.at(index)->obj->is_folder(),
                        .isDirectoryOpen = parentFolder.at(index)->obj->is_folder() ? parentFolder.at(index)->obj->get_folder().isFolderOpen : false
                    };
                },
                .getIndexOfObjInList = [&](const GUIStuff::TreeListing::ParentObjectIDPair& idPair) {
                    return world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(idPair.parent)->get_folder().folderList->get(idPair.object)->pos;
                },
                .setDirectoryOpen = [&](NetObjID netID, bool newDirectoryOpen) {
                    world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(netID)->get_folder().isFolderOpen = newDirectoryOpen;
                },
                .drawNonDirectoryObjIconGUI = [&](NetObjID netID) {
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            .padding = CLAY_PADDING_ALL(2),
                        },
                    }) {
                        gui.svg_icon("ico", "data/icons/layer.svg");
                    }
                    return false;
                },
                .drawObjGUI = [&](const GUIStuff::TreeListing::ParentObjectIDPair& idPair) {
                    auto tempPtr = world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(idPair.object);
                    bool isButtonClicked = false;
                    if(Clay_Hovered() && gui.io->mouse.leftClick >= 2 && !tempPtr->is_folder() && selectionData.objsSelected.contains(idPair.object) && !gui.io->key.leftCtrl && !gui.io->key.leftShift)
                        layerMan.editingLayer = tempPtr;
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_FIXED(GUIStuff::TreeListing::ENTRY_HEIGHT), .height = CLAY_SIZING_FIXED(GUIStuff::TreeListing::ENTRY_HEIGHT)},
                            .padding = CLAY_PADDING_ALL(2),
                        },
                    }) {
                        if(tempPtr.get_net_id() == layerMan.editingLayer.get_net_id())
                            gui.svg_icon("edit ico", "data/icons/pencil.svg");
                    }
                    CLAY_AUTO_ID({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                            .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                        },
                    }) {
                        gui.text_label(tempPtr->get_name());
                    }
                    if(gui.svg_icon_button_transparent("visible button", tempPtr->get_visible() ? "data/icons/eyeopen.svg" : "data/icons/eyeclose.svg", false, GUIStuff::TreeListing::ENTRY_HEIGHT)) {
                        tempPtr->set_visible(layerMan, !tempPtr->get_visible());
                        isButtonClicked = true;
                    }
                    if(gui.svg_icon_button_transparent("delete button", "data/icons/trash.svg", false, GUIStuff::TreeListing::ENTRY_HEIGHT)) {
                        toDeleteParent = idPair.parent;
                        toDeleteObject = idPair.object;
                        isButtonClicked = true;
                    }
                    return isButtonClicked;
                },
                .moveObjectsToListAtIndex = [&](NetObjID listObj, size_t index, const std::vector<GUIStuff::TreeListing::ParentObjectIDPair>& objsToInsert) {
                    world.netObjMan.send_multi_update_messsage([&]() {
                        std::unordered_map<NetObjID, std::unordered_set<NetObjID>> toEraseMap;
                        uint32_t newIndex = index;
                        std::vector<NetObjOwnerPtr<DrawingProgramLayerListItem>> toInsertObjPtrs;
                        auto& listPtr = world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(listObj)->get_folder().folderList;

                        for(size_t i = 0; i < objsToInsert.size(); i++) {
                            toEraseMap[objsToInsert[i].parent].emplace(objsToInsert[i].object);
                            if(newIndex != 0 && objsToInsert[i].parent == listObj && listPtr->get(objsToInsert[i].object)->pos <= index)
                                newIndex--;
                        }

                        for(auto& [listToEraseFrom, setToErase] : toEraseMap) {
                            auto& listToEraseFromPtr = world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(listToEraseFrom)->get_folder().folderList;
                            listToEraseFromPtr->erase_unordered_set(listToEraseFromPtr, setToErase, &toInsertObjPtrs);
                        }

                        std::vector<std::pair<uint32_t, NetObjOwnerPtr<DrawingProgramLayerListItem>>> toInsert;
                        for(uint32_t i = 0; i < objsToInsert.size(); i++) {
                            auto it = std::find_if(toInsertObjPtrs.begin(), toInsertObjPtrs.end(), [id = objsToInsert[i].object](NetObjOwnerPtr<DrawingProgramLayerListItem>& objPtr) {
                                return objPtr.get_net_id() == id;
                            });
                            toInsert.emplace_back(newIndex + i, std::move(*it));
                            toInsert.back().second.reassign_ids();
                        }
                        listPtr->insert_ordered_list_and_send_create(listPtr, toInsert);
                    }, NetObjManager::SendUpdateType::SEND_TO_ALL, nullptr);
                }
            }, selectionData);


            if(selectionData.objsSelected != oldSelection) {
                oldSelection = selectionData.objsSelected;
                if(selectionData.objsSelected.size() == 1) {
                    NetObjTemporaryPtr<DrawingProgramLayerListItem> tempPtr = world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(*selectionData.objsSelected.begin());
                    if(tempPtr) {
                        nameToEdit = tempPtr->get_name();
                        alphaValToEdit = tempPtr->get_alpha();
                        blendModeValToEdit = static_cast<size_t>(tempPtr->get_blend_mode());
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
            }

            if(toDeleteObject.has_value()) {
                auto& folderList = world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(toDeleteParent.value())->get_folder().folderList;
                folderList->erase(folderList, toDeleteObject.value());
            }
        }

        gui.left_to_right_line_layout([&]() {
            bool addByEnter = false;
            gui.input_text("input new name", &nameForNew, true, [&](GUIStuff::SelectionHelper& s) {
                addByEnter = s.selected && gui.io->key.enter;
            });
            
            if(gui.svg_icon_button("create new layer", "data/icons/plusbold.svg", false, gui.SMALL_BUTTON_SIZE) || addByEnter) {
                auto newLayerObjInfo = create_in_proper_position(new DrawingProgramLayerListItem(world.netObjMan, nameForNew, false));
                layerMan.editingLayer = newLayerObjInfo->obj;
            }
            else if(gui.svg_icon_button("create new folder", "data/icons/folderbold.svg", false, gui.SMALL_BUTTON_SIZE)) { 
                auto newFolderObjInfo = create_in_proper_position(new DrawingProgramLayerListItem(world.netObjMan, nameForNew, true));
                newFolderObjInfo->obj->get_folder().isFolderOpen = true;
            }
        });

        if(selectionData.objsSelected.size() == 1) {
            NetObjTemporaryPtr<DrawingProgramLayerListItem> tempPtr = world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(*selectionData.objsSelected.begin());
            if(tempPtr) {
                gui.text_label_centered(tempPtr->is_folder() ? "Edit Layer Folder" : "Edit Layer");
                gui.input_text_field("input edit name", "Name", &nameToEdit);
                tempPtr->set_name(world.delayedUpdateObjectManager, nameToEdit);
                gui.slider_scalar_field("input alpha slider", "Alpha", &alphaValToEdit, 0.0f, 1.0f, 2);
                tempPtr->set_alpha(layerMan, alphaValToEdit);
                gui.left_to_right_line_layout([&]() {
                    gui.text_label("Blend Mode");
                    gui.dropdown_select("input blend mode", &blendModeValToEdit, get_blend_mode_name_list(), 200.0f, [&hoveringOverDropdown]() {
                        if(Clay_Hovered())
                            hoveringOverDropdown = true;
                    });
                });
                tempPtr->set_blend_mode(layerMan, static_cast<SerializedBlendMode>(blendModeValToEdit));
            }
        }

        gui.pop_id();
    }
}

NetworkingObjects::NetObjOrderedListObjectInfoPtr<DrawingProgramLayerListItem> DrawingProgramLayerManagerGUI::try_to_create_in_proper_position(DrawingProgramLayerListItem* newItem) {
    using namespace NetworkingObjects;
    auto& world = layerMan.drawP.world;
    if(selectionData.orderedObjsSelected.empty())
        return nullptr;
    auto& backObjParentObjectPair = selectionData.orderedObjsSelected.back();
    NetObjTemporaryPtr<DrawingProgramLayerListItem> parentPtr = world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(backObjParentObjectPair.parent);
    if(!parentPtr)
        return nullptr;
    NetObjTemporaryPtr<DrawingProgramLayerListItem> objectPtr = world.netObjMan.get_obj_temporary_ref_from_id<DrawingProgramLayerListItem>(backObjParentObjectPair.object);
    if(!objectPtr)
        return nullptr;
    if(objectPtr->is_folder()) {
        objectPtr->get_folder().isFolderOpen = true;
        return objectPtr->get_folder().folderList->insert_and_send_create(objectPtr->get_folder().folderList, 0, newItem);
    }
    uint32_t insertPos = parentPtr->get_folder().folderList->get(objectPtr.get_net_id())->pos;
    return parentPtr->get_folder().folderList->insert_and_send_create(parentPtr->get_folder().folderList, insertPos, newItem);
}

NetworkingObjects::NetObjOrderedListObjectInfoPtr<DrawingProgramLayerListItem> DrawingProgramLayerManagerGUI::create_in_proper_position(DrawingProgramLayerListItem* newItem) {
    auto toRet = try_to_create_in_proper_position(newItem);
    if(!toRet)
        return layerMan.layerTreeRoot->get_folder().folderList->insert_and_send_create(layerMan.layerTreeRoot->get_folder().folderList, 0, newItem);
    return toRet;
}
