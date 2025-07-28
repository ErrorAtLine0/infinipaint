#pragma once
#include "../SharedTypes.hpp"
#include "DrawComponent.hpp"
#include "../CollabTextBox/CollabTextBoxCursor.hpp"
#include "../CoordSpaceHelper.hpp"

#ifndef IS_SERVER
    #include <include/core/SkPath.h>
    #include "../CollabTextBox/CollabTextBox.hpp"
#endif

class DrawTextBox : public DrawComponent {
    public:
        virtual DrawComponentType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;

        // User input data
        struct Data {
            Vector4f textColor;
            float textSize = -100000.0f;
            Vector2f p1;
            Vector2f p2;
            std::string currentText;
            CollabTextBox::Cursor cursor;
            bool editing = false;
            bool operator==(const Data& o) const = default;
        } d;

#ifndef IS_SERVER
        virtual std::shared_ptr<DrawComponent> copy() const override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData) override;
        virtual void initialize_draw_data(DrawingProgram& drawP) override;
        virtual bool collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) override;
        virtual void update(DrawingProgram& drawP) override;
        virtual void create_collider() override;
        virtual void update_from_delayed_ptr() override;
        void init_text_box(DrawingProgram& drawP);
        void update_contained_string(DrawingProgram& drawP);
        void set_textbox_string(const std::string& str);
        Vector2f get_mouse_pos(DrawingProgram& drawP);

        virtual SCollision::AABB<float> get_obj_coord_bounds() const override;

        bool textboxUpdate = false;
        CollabTextBox::Editor textBox;
        SCollision::BVHContainer<float> collisionTree;
#endif
};
