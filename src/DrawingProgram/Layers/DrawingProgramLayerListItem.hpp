#pragma once
#include <Helpers/NetworkingObjects/NetObjManager.hpp>
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include <Helpers/NetworkingObjects/DelayUpdateSerializedClassManager.hpp>
#include "DrawingProgramLayerFolder.hpp"
#include "DrawingProgramLayer.hpp"
#include "SerializedBlendMode.hpp"
#include "../../CanvasComponents/CanvasComponentContainer.hpp"

class DrawingProgramLayerFolder;
class DrawingProgramLayer;
class World;
class DrawingProgramLayerManager;

class DrawingProgramLayerListItem {
    public:
        DrawingProgramLayerListItem();
        DrawingProgramLayerListItem(NetworkingObjects::NetObjManager& netObjMan, const std::string& initName, bool isFolder);
        bool is_folder() const;
        DrawingProgramLayerFolder& get_folder() const;
        DrawingProgramLayer& get_layer() const;
        static void register_class(World& w);
        void reassign_netobj_ids_call();
        void set_component_list_callbacks(DrawingProgramLayerManager &layerMan);
        void commit_update_dont_invalidate_cache(DrawingProgramLayerManager& layerMan);
        void draw(SkCanvas* canvas, const DrawData& drawData) const;
        void set_name(NetworkingObjects::DelayUpdateSerializedClassManager& delayedNetObjMan, const std::string& newName) const;
        const std::string& get_name() const;
        void get_flattened_component_list(std::vector<CanvasComponentContainer::ObjInfoSharedPtr>& objList) const;
        void get_flattened_layer_list(std::vector<DrawingProgramLayerListItem*>& objList);

        void set_alpha(DrawingProgramLayerManager& layerMan, float newAlpha) const;
        float get_alpha() const;

        void set_visible(DrawingProgramLayerManager& layerMan, bool newVisible) const;
        bool get_visible() const;

        void set_blend_mode(DrawingProgramLayerManager& layerMan, SerializedBlendMode newBlendMode) const;
        SerializedBlendMode get_blend_mode() const;
    private:
        static void write_constructor_func(const NetworkingObjects::NetObjTemporaryPtr<DrawingProgramLayerListItem>& o, cereal::PortableBinaryOutputArchive& a);
        static void read_constructor_func(const NetworkingObjects::NetObjTemporaryPtr<DrawingProgramLayerListItem>& o, cereal::PortableBinaryInputArchive& a, const std::shared_ptr<NetServer::ClientData>& c);
        std::unique_ptr<DrawingProgramLayerFolder> folderData;
        std::unique_ptr<DrawingProgramLayer> layerData;
        struct NameData {
            std::string name;
            template <typename Archive> void serialize(Archive& a) {
                a(name);
            }
        };
        struct DisplayData {
            float alpha = 1.0f;
            bool visible = true;
            SerializedBlendMode blendMode = SerializedBlendMode::SRC_OVER;
            template <typename Archive> void serialize(Archive& a) {
                a(alpha, visible, blendMode);
            }
        };
        NetworkingObjects::NetObjOwnerPtr<NameData> nameData;
        NetworkingObjects::NetObjOwnerPtr<DisplayData> displayData;
};
