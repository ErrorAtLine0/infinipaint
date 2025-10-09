#pragma once
#include "ResourceDisplay.hpp"
#include <modules/svg/include/SkSVGDOM.h>

class FileResourceDisplay : public ResourceDisplay {
    public:
        virtual void update(World& w) override;
        virtual bool load(ResourceManager& rMan, const std::string& fileName, const std::string& fileData) override;
        virtual bool update_draw() const override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData, const SkRect& imRect) override;
        virtual Vector2f get_dimensions() const override;
        virtual float get_dimension_scale() const override;
        virtual Type get_type() const override;
    private:
        std::string fileName;
};
