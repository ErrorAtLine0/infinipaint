#pragma once
#include <include/core/SkCanvas.h>
#include <include/core/SkRect.h>
#include <Eigen/Dense>

using namespace Eigen;

struct DrawData;
class World;

class ResourceDisplay {
    public:
        virtual void update(World& w) = 0;
        virtual bool load(const std::string& fileName, const std::string& fileData) = 0;
        virtual bool update_draw() const = 0;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData, const SkRect& imRect) = 0;
        virtual Vector2f get_dimensions() const = 0;
        virtual float get_dimension_scale() const = 0;
        virtual ~ResourceDisplay();
};
