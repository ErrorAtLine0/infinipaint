#pragma once
#include <include/core/SkCanvas.h>
#include <include/core/SkRect.h>
#include <Eigen/Dense>
#include "../SharedTypes.hpp"
#include <Helpers/SCollision.hpp>
#include "../CoordSpaceHelper.hpp"

using namespace Eigen;

struct DrawData;
class World;
class ResourceManager;

class ResourceDisplay {
    public:
        enum class Type {
            FILE,
            IMAGE,
            SVG
        };
        virtual void update(World& w) = 0;
        virtual bool load(ResourceManager& rMan, const std::string& fileName, const std::shared_ptr<std::string>& fileData) = 0;
        virtual bool update_draw() const = 0;
        virtual void draw(SkCanvas* canvas, const DrawData& drawData, const SkRect& imRect) = 0;
        virtual Vector2f get_dimensions() const = 0;
        virtual float get_dimension_scale() const = 0;
        virtual Type get_type() const = 0;
        virtual void camera_view_update(const CoordSpaceHelper& compCoords, const SCollision::AABB<WorldScalar>& compAABB, const DrawData& drawData, const SkRect& imRect);
        virtual ~ResourceDisplay();
};
