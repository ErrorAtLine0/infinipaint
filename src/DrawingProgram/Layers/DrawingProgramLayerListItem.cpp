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

void DrawingProgramLayerListItem::reassign_netobj_ids_call() {
    if(folderData)
        folderData->folderList.reassign_ids();
    else
        layerData->components.reassign_ids();
    nameData.reassign_ids();
    displayData.reassign_ids();
}

void DrawingProgramLayerListItem::set_component_list_callbacks(DrawingProgramLayerManager &layerMan) {
    if(folderData)
        folderData->set_component_list_callbacks(layerMan);
    else
        layerData->set_component_list_callbacks(*this, layerMan);
}

void DrawingProgramLayerListItem::commit_update_dont_invalidate_cache(DrawingProgramLayerManager& layerMan) {
    if(folderData)
        folderData->commit_update_dont_invalidate_cache(layerMan);
    else
        layerData->commit_update_dont_invalidate_cache(layerMan);
}

void DrawingProgramLayerListItem::get_flattened_component_list(std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& objList) const {
    if(folderData)
        folderData->get_flattened_component_list(objList);
    else
        layerData->get_flattened_component_list(objList);
}

void DrawingProgramLayerListItem::get_flattened_layer_list(std::vector<DrawingProgramLayerListItem*>& objList) {
    if(folderData) {
        for(auto& c : folderData->folderList->get_data())
            c->obj->get_flattened_layer_list(objList);
    }
    else
        objList.emplace_back(this);
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
