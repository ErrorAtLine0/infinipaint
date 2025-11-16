#pragma once
#include "../SharedTypes.hpp"
#include "DrawComponent.hpp"
#include "../CoordSpaceHelper.hpp"
#include "../RichText/TextBox.hpp"

#ifndef IS_SERVER
    #include <include/core/SkPath.h>
#endif

class DrawTextBox : public DrawComponent {
    public:
        constexpr static float TEXTBOX_PADDING = 5.0f;

        DrawTextBox();
        virtual DrawComponentType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void save_file(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) override;

        // User input data
        struct Data {
            Vector2f p1;
            Vector2f p2;
            bool editing = false; // Should probably move this out of the Data struct
            bool operator==(const Data& o) const = default;
            bool operator!=(const Data& o) const = default;
        } d;

#ifndef IS_SERVER
        virtual std::shared_ptr<DrawComponent> copy() const override;
        virtual std::shared_ptr<DrawComponent> deep_copy() const override;

        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual void initialize_draw_data(DrawingProgram& drawP) override;
        virtual bool collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) override;
        virtual void update(DrawingProgram& drawP) override;
        void create_collider();
        virtual void update_from_delayed_ptr(const std::shared_ptr<DrawComponent>& delayedUpdatePtr) override;
        void init_text_box(DrawingProgram& drawP);
        Vector2f get_mouse_pos(DrawingProgram& drawP);

        virtual SCollision::AABB<float> get_obj_coord_bounds() const override;

        std::shared_ptr<RichText::TextBox> textBox;
        std::shared_ptr<RichText::TextBox::Cursor> cursor;
        SCollision::BVHContainer<float> collisionTree;
#endif
};
