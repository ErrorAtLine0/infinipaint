#pragma once
#include "ResourceDisplay.hpp"
#include <include/core/SkImage.h>
#include <vector>
#include <modules/svg/include/SkSVGDOM.h>

class SvgResourceDisplay : public ResourceDisplay {
    public:
        virtual void update(World& w) override;
        virtual bool load(ResourceManager& rMan, const std::string& fileName, const std::shared_ptr<std::string>& fileData) override;
        virtual bool update_draw() const override;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData, const SkRect& imRect) override;
        virtual Vector2f get_dimensions() const override;
        virtual float get_dimension_scale() const override;
        virtual Type get_type() const override;
    private:
        sk_sp<SkSVGDOM> svgDom;
        bool mustUpdateDraw = false;
        SkSize svgRootSize;
};
