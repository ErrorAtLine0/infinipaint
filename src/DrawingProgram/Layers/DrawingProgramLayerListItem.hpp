#pragma once
#include <Helpers/NetworkingObjects/NetObjManager.hpp>
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include <Helpers/NetworkingObjects/DelayUpdateSerializedClassManager.hpp>
#include <unordered_set>
#include "DrawingProgramLayerFolder.hpp"
#include "DrawingProgramLayer.hpp"
#include "SerializedBlendMode.hpp"
#include "../../CanvasComponents/CanvasComponentContainer.hpp"

class DrawingProgramLayerFolder;
class DrawingProgramLayer;
class World;
class DrawingProgramLayerManager;

struct DrawingProgramLayerListItemMetaInfo {
    std::string name;
    float alpha = 1.0f;
    SerializedBlendMode blendMode = SerializedBlendMode::SRC_OVER;
    bool operator==(const DrawingProgramLayerListItemMetaInfo&) const = default;
};

struct DrawingProgramLayerListItemUndoData {
    WorldUndoManager::UndoObjectID undoID;
    DrawingProgramLayerListItemMetaInfo metaInfo;
    std::optional<std::vector<DrawingProgramLayerListItemUndoData>> folderData;
    std::optional<std::vector<DrawingProgramComponentUndoData>> layerData;
    void scale_up(const WorldScalar& scaleUpAmount);
};

class DrawingProgramLayerListItem {
    public:
        DrawingProgramLayerListItem();
        DrawingProgramLayerListItem(NetworkingObjects::NetObjManager& netObjMan, const std::string& initName, bool isFolder);
        DrawingProgramLayerListItem(World& w, const DrawingProgramLayerListItemUndoData& undoData);
        DrawingProgramLayerListItemUndoData get_undo_data(WorldUndoManager& u) const;
        bool is_folder() const;
        DrawingProgramLayerFolder& get_folder() const;
        DrawingProgramLayer& get_layer() const;
        static void register_class(World& w);
        void reassign_netobj_ids_call();
        void set_to_erase();
        void set_component_list_callbacks(DrawingProgramLayerManager &layerMan);
        void draw(SkCanvas* canvas, const DrawData& drawData) const;
        void set_name(NetworkingObjects::DelayUpdateSerializedClassManager& delayedNetObjMan, const std::string& newName) const;
        const std::string& get_name() const;
        void get_flattened_component_list(std::vector<CanvasComponentContainer::ObjInfo*>& objList) const;
        void get_flattened_layer_list(std::vector<DrawingProgramLayerListItem*>& objList);
        void scale_up(const WorldScalar& scaleUpAmount);
        void get_used_resources(std::unordered_set<NetworkingObjects::NetObjID>& resourceSet) const;

        void set_alpha(DrawingProgramLayerManager& layerMan, float newAlpha) const;
        float get_alpha() const;

        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version, DrawingProgramLayerManager& layerMan);
        void save_file(cereal::PortableBinaryOutputArchive& a) const;

        void set_visible(DrawingProgramLayerManager& layerMan, bool newVisible) const;
        bool get_visible() const;

        void set_blend_mode(DrawingProgramLayerManager& layerMan, SerializedBlendMode newBlendMode) const;
        SerializedBlendMode get_blend_mode() const;

        void set_metainfo(DrawingProgramLayerManager& layerMan, const DrawingProgramLayerListItemMetaInfo& metaInfo);
        DrawingProgramLayerListItemMetaInfo get_metainfo() const;

        uint32_t get_component_count() const;
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
