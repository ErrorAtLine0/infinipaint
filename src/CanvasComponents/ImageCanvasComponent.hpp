#pragma once
#include "../SharedTypes.hpp"
#include "CanvasComponent.hpp"
#include "../CoordSpaceHelper.hpp"
#include <include/core/SkPath.h>

class ImageCanvasComponent : public CanvasComponent {
    public:
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void save_file(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) override;
        virtual CanvasComponentType get_type() const override;
        std::unique_ptr<CanvasComponent> get_data_copy() const override;
        virtual void set_data_from(const CanvasComponent& other) override;
        virtual void remap_resource_ids(const std::unordered_map<NetworkingObjects::NetObjID, NetworkingObjects::NetObjID>& resourceOldToNewMap) override;
        virtual void get_used_resources(std::unordered_set<NetworkingObjects::NetObjID>& resourceSet) const override;

        void draw_download_progress_bar(SkCanvas* canvas, const DrawData& drawData, float progress) const;

        // User input data
        struct Data {
            Vector2f p1;
            Vector2f p2;
            Vector2f cropP1 = {0.0f, 0.0f};
            Vector2f cropP2 = {1.0f, 1.0f};

            NetworkingObjects::NetObjID imageID;

            bool editing = false;
        } d;

        virtual void update(DrawingProgram& drawP) override;

        bool contains_actual_image();

    private:
        virtual void draw(SkCanvas* canvas, const DrawData& drawData, const std::shared_ptr<void>& predrawData) const override;
        virtual void initialize_draw_data(DrawingProgram& drawP) override;
        virtual bool collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) const override;
        void create_collider();

        virtual SCollision::AABB<float> get_obj_coord_bounds() const override;

        SCollision::BVHContainer<float> collisionTree;
};

