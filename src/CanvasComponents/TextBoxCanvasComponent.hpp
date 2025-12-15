#pragma once
#include "../SharedTypes.hpp"
#include "CanvasComponent.hpp"
#include "../CoordSpaceHelper.hpp"
#include "../RichText/TextBox.hpp"
#include <include/core/SkPath.h>

class TextBoxCanvasComponent : public CanvasComponent {
    public:
        constexpr static float TEXTBOX_PADDING = 5.0f;

        TextBoxCanvasComponent();
        virtual CanvasComponentType get_type() const override;
        virtual void save(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load(cereal::PortableBinaryInputArchive& a) override;
        virtual void save_file(cereal::PortableBinaryOutputArchive& a) const override;
        virtual void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) override;
        virtual void set_data_from(const CanvasComponent& other) override;

        // User input data
        struct Data {
            Vector2f p1;
            Vector2f p2;
            bool editing = false; // Should probably move this out of the Data struct
            bool operator==(const Data& o) const = default;
            bool operator!=(const Data& o) const = default;
        } d;

    private:
        friend class TextBoxEditTool;

        virtual void draw(SkCanvas* canvas, const DrawData& drawData) const override;
        virtual void initialize_draw_data(DrawingProgram& drawP) override;
        virtual bool collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) const override;
        void create_collider();
        void init_text_box(DrawingProgram& drawP);
        Vector2f get_mouse_pos(DrawingProgram& drawP) const;

        virtual SCollision::AABB<float> get_obj_coord_bounds() const override;

        std::shared_ptr<RichText::TextBox> textBox;
        std::shared_ptr<RichText::TextBox::Cursor> cursor;
        SCollision::BVHContainer<float> collisionTree;
};
