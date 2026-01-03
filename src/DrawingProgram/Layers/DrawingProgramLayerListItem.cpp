#include "DrawingProgramLayerListItem.hpp"
#include "DrawingProgramLayerFolder.hpp"
#include <cereal/types/string.hpp>
#include "DrawingProgramLayer.hpp"
#include "../../World.hpp"
#include "SerializedBlendMode.hpp"

using namespace NetworkingObjects;

DrawingProgramLayerListItem::DrawingProgramLayerListItem() {}

DrawingProgramLayerListItem::DrawingProgramLayerListItem(NetworkingObjects::NetObjManager& netObjMan, const std::string& initName, bool isFolder) {
    if(isFolder) {
        folderData = std::make_unique<DrawingProgramLayerFolder>();
        folderData->folderList = netObjMan.make_obj<NetworkingObjects::NetObjOrderedList<DrawingProgramLayerListItem>>();
    }
    else {
        layerData = std::make_unique<DrawingProgramLayer>();
        layerData->components = netObjMan.make_obj<CanvasComponentContainer::NetList>();
    }
    nameData = netObjMan.make_obj<NameData>();
    nameData->name = initName;
    displayData = netObjMan.make_obj<DisplayData>();
}

DrawingProgramLayerListItem::DrawingProgramLayerListItem(World& w, const DrawingProgramLayerListItemUndoData& initData) {
    if(initData.folderData) {
        auto& initFolderList = initData.folderData.value();
        folderData = std::make_unique<DrawingProgramLayerFolder>();
        folderData->folderList = w.netObjMan.make_obj<NetworkingObjects::NetObjOrderedList<DrawingProgramLayerListItem>>();
        for(auto& initFolder : initFolderList) {
            auto it = folderData->folderList->emplace_back_direct(folderData->folderList, w, initFolder);
            w.undo.register_new_netid_to_existing_undoid(initFolder.undoID, it->obj.get_net_id());
        }
    }
    else {
        auto& initComponentList = initData.layerData.value();
        layerData = std::make_unique<DrawingProgramLayer>();
        layerData->components = w.netObjMan.make_obj<CanvasComponentContainer::NetList>();
        for(auto& initComponent : initComponentList) {
            auto it = layerData->components->emplace_back_direct(layerData->components, w.netObjMan, *initComponent.copyData);
            w.undo.register_new_netid_to_existing_undoid(initComponent.undoID, it->obj.get_net_id());
        }
    }
    nameData = w.netObjMan.make_obj<NameData>();
    nameData->name = initData.metaInfo.name;
    displayData = w.netObjMan.make_obj<DisplayData>();
    displayData->alpha = initData.metaInfo.alpha;
    displayData->blendMode = initData.metaInfo.blendMode;
}

DrawingProgramLayerListItemUndoData DrawingProgramLayerListItem::get_undo_data(WorldUndoManager& u) const {
    DrawingProgramLayerListItemUndoData toRet;
    toRet.metaInfo.name = nameData->name;
    toRet.metaInfo.blendMode = displayData->blendMode;
    toRet.metaInfo.alpha = displayData->alpha;
    if(folderData) {
        toRet.folderData = std::vector<DrawingProgramLayerListItemUndoData>();
        auto& folderListToRet = toRet.folderData.value();
        for(auto& f : *folderData->folderList) {
            folderListToRet.emplace_back(f.obj->get_undo_data(u));
            folderListToRet.back().undoID = u.get_undoid_from_netid(f.obj.get_net_id());
        }
    }
    else {
        toRet.layerData = std::vector<DrawingProgramComponentUndoData>();
        auto& layerListToRet = toRet.layerData.value();
        for(auto& c : *layerData->components)
            layerListToRet.emplace_back(u.get_undoid_from_netid(c.obj.get_net_id()), c.obj->get_data_copy());
    }
    return toRet;
}

void DrawingProgramLayerListItem::reassign_netobj_ids_call() {
    if(folderData)
        folderData->folderList.reassign_ids();
    else
        layerData->components.reassign_ids();
    nameData.reassign_ids();
    displayData.reassign_ids();
}

void DrawingProgramLayerListItem::set_to_erase() {
    if(folderData)
        folderData->set_to_erase();
    else
        layerData->set_to_erase();
}

void DrawingProgramLayerListItem::set_component_list_callbacks(DrawingProgramLayerManager &layerMan) {
    if(folderData)
        folderData->set_component_list_callbacks(layerMan);
    else
        layerData->set_component_list_callbacks(*this, layerMan);
}

void DrawingProgramLayerListItem::get_flattened_component_list(std::vector<CanvasComponentContainer::ObjInfo*>& objList) const {
    if(folderData)
        folderData->get_flattened_component_list(objList);
    else
        layerData->get_flattened_component_list(objList);
}

void DrawingProgramLayerListItem::get_flattened_layer_list(std::vector<DrawingProgramLayerListItem*>& objList) {
    if(folderData) {
        for(auto& c : *folderData->folderList)
            c.obj->get_flattened_layer_list(objList);
    }
    else
        objList.emplace_back(this);
}

uint32_t DrawingProgramLayerListItem::get_component_count() const {
    if(folderData) {
        uint32_t toRet = 0;
        for(auto& c : *folderData->folderList)
            toRet += c.obj->get_component_count();
        return toRet;
    }
    else
        return layerData->components->size();
}

void DrawingProgramLayerListItem::scale_up(const WorldScalar& scaleUpAmount) {
    if(folderData)
        folderData->scale_up(scaleUpAmount);
    else
        layerData->scale_up(scaleUpAmount);
}

bool DrawingProgramLayerListItem::is_folder() const {
    return folderData != nullptr;
}

DrawingProgramLayerFolder& DrawingProgramLayerListItem::get_folder() const {
    return *folderData;
}

DrawingProgramLayer& DrawingProgramLayerListItem::get_layer() const {
    return *layerData;
}

void DrawingProgramLayerListItem::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version, DrawingProgramLayerManager& layerMan) {
    nameData = layerMan.drawP.world.netObjMan.make_obj<NameData>();
    a(*nameData);

    displayData = layerMan.drawP.world.netObjMan.make_obj<DisplayData>();
    a(*displayData);

    bool isFolder;
    a(isFolder);
    if(isFolder) {
        folderData = std::make_unique<DrawingProgramLayerFolder>();
        folderData->load_file(a, version, layerMan);
    }
    else {
        layerData = std::make_unique<DrawingProgramLayer>();
        layerData->load_file(a, version, layerMan);
    }
}

void DrawingProgramLayerListItem::save_file(cereal::PortableBinaryOutputArchive& a) const {
    a(*nameData);
    a(*displayData);
    a(static_cast<bool>(folderData));
    if(folderData)
        folderData->save_file(a);
    else
        layerData->save_file(a);
}

void DrawingProgramLayerListItem::get_used_resources(std::unordered_set<NetworkingObjects::NetObjID>& resourceSet) const {
    if(folderData)
        folderData->get_used_resources(resourceSet);
    else
        layerData->get_used_resources(resourceSet);
}

void DrawingProgramLayerListItem::draw(SkCanvas* canvas, const DrawData& drawData) const {
    if(displayData->visible) {
        SkPaint layerPaint;
        layerPaint.setAlphaf(displayData->alpha);
        layerPaint.setBlendMode(serialized_blend_mode_to_sk_blend_mode(displayData->blendMode));
        canvas->saveLayer(nullptr, &layerPaint);
        if(folderData)
            folderData->draw(canvas, drawData);
        else
            layerData->draw(canvas, drawData);
        canvas->restore();
    }
}

void DrawingProgramLayerListItem::set_name(NetworkingObjects::DelayUpdateSerializedClassManager& delayedNetObjMan, const std::string& newName) const {
    if(nameData && nameData->name != newName) {
        nameData->name = newName;
        delayedNetObjMan.send_update_to_all<NameData>(nameData, false);
    }
}

const std::string& DrawingProgramLayerListItem::get_name() const {
    return nameData->name;
}

void DrawingProgramLayerListItem::set_alpha(DrawingProgramLayerManager& layerMan, float newAlpha) const {
    if(displayData && displayData->alpha != newAlpha) {
        displayData->alpha = newAlpha;
        layerMan.drawP.drawCache.clear_own_cached_surfaces();
        layerMan.drawP.world.delayedUpdateObjectManager.send_update_to_all<DisplayData>(displayData, false);
    }
}

float DrawingProgramLayerListItem::get_alpha() const {
    return displayData->alpha;
}

void DrawingProgramLayerListItem::set_visible(DrawingProgramLayerManager& layerMan, bool newVisible) const {
    if(displayData && displayData->visible != newVisible) {
        displayData->visible = newVisible;
        layerMan.drawP.drawCache.clear_own_cached_surfaces();
        layerMan.drawP.world.delayedUpdateObjectManager.send_update_to_all<DisplayData>(displayData, false);
    }
}

bool DrawingProgramLayerListItem::get_visible() const {
    return displayData->visible;
}

void DrawingProgramLayerListItem::set_blend_mode(DrawingProgramLayerManager& layerMan, SerializedBlendMode newBlendMode) const {
    if(displayData && displayData->blendMode != newBlendMode) {
        displayData->blendMode = newBlendMode;
        layerMan.drawP.drawCache.clear_own_cached_surfaces();
        layerMan.drawP.world.delayedUpdateObjectManager.send_update_to_all<DisplayData>(displayData, false);
    }
}

SerializedBlendMode DrawingProgramLayerListItem::get_blend_mode() const {
    return displayData->blendMode;
}

void DrawingProgramLayerListItem::register_class(World& w) {
    w.netObjMan.register_class<DrawingProgramLayerListItem, DrawingProgramLayerListItem, DrawingProgramLayerListItem, DrawingProgramLayerListItem>({
        .writeConstructorFuncClient = write_constructor_func,
        .readConstructorFuncClient = read_constructor_func,
        .readUpdateFuncClient = nullptr,
        .writeConstructorFuncServer = write_constructor_func,
        .readConstructorFuncServer = read_constructor_func,
        .readUpdateFuncServer = nullptr,
    });
    w.delayedUpdateObjectManager.register_class<NameData>(w.netObjMan);
    w.delayedUpdateObjectManager.register_class<DisplayData>(w.netObjMan, NetworkingObjects::DelayUpdateSerializedClassManager::CustomConstructors<DisplayData>{
        .postUpdateFunc = [&](DisplayData& o) {
            w.drawProg.drawCache.clear_own_cached_surfaces();
        }
    });
}

void DrawingProgramLayerListItem::write_constructor_func(const NetworkingObjects::NetObjTemporaryPtr<DrawingProgramLayerListItem>& o, cereal::PortableBinaryOutputArchive& a) {
    a(o->is_folder());
    if(o->is_folder())
        o->folderData->folderList.write_create_message(a);
    else
        o->layerData->components.write_create_message(a);
    o->nameData.write_create_message(a);
    o->displayData.write_create_message(a);
}

void DrawingProgramLayerListItem::read_constructor_func(const NetworkingObjects::NetObjTemporaryPtr<DrawingProgramLayerListItem>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c) {
    bool isFolder;
    a(isFolder);
    if(isFolder) {
        o->folderData = std::make_unique<DrawingProgramLayerFolder>();
        o->folderData->folderList = o.get_obj_man()->read_create_message<NetworkingObjects::NetObjOrderedList<DrawingProgramLayerListItem>>(a, c);
    }
    else {
        o->layerData = std::make_unique<DrawingProgramLayer>();
        o->layerData->components = o.get_obj_man()->read_create_message<CanvasComponentContainer::NetList>(a, c);
    }
    o->nameData = o.get_obj_man()->read_create_message<NameData>(a, c);
    o->displayData = o.get_obj_man()->read_create_message<DisplayData>(a, c);
}
