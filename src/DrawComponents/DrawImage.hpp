#pragma once
#include "../SharedTypes.hpp"
#include "DrawComponent.hpp"
#include "../CoordSpaceHelper.hpp"

#ifndef IS_SERVER
#include <include/core/SkPath.h>
#endif

class DrawImage : public DrawComponent {
    public:
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual DrawComponentType get_type() const override;
        virtual void get_used_resources(std::unordered_set<ServerClientID>& v) const override;
        virtual void remap_resource_ids(std::unordered_map<ServerClientID, ServerClientID>& newIDs) override;

        // User input data
        struct Data {
            Vector2f p1;
            Vector2f p2;

            ServerClientID imageID;
        } d;

#ifndef IS_SERVER
        virtual std::shared_ptr<DrawComponent> copy() const override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual void initialize_draw_data(DrawingProgram& drawP) override;
        virtual void update(DrawingProgram& drawP) override;
        virtual bool collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) override;
        void create_draw_data();
        virtual void create_collider() override;
        virtual void update_from_delayed_ptr() override;

        virtual SCollision::AABB<float> get_obj_coord_bounds() const override;

        SkRect imRect;

        SCollision::BVHContainer<float> collisionTree;
#endif
};

