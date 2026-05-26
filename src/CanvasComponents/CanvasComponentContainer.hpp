/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include "../CoordSpaceHelper.hpp"
#include <Helpers/NetworkingObjects/NetObjManagerTypeList.hpp>
#include "CanvasComponentType.hpp"
#include "../VersionConstants.hpp"
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include <Helpers/NetworkingObjects/NetObjOwnerPtr.hpp>
#include "CanvasComponentAllocator.hpp"
#include "../WorldUndoManager.hpp"

class DrawingProgram;
class DrawingProgramLayerListItem;
struct DrawingProgramCacheBVHNode;

enum class CanvasComponentEraseDetailResult {
    NO_CHANGE,
    CHANGED,
    REMOVED
};

class CanvasComponentContainer {
    public:
        typedef NetworkingObjects::NetObjOrderedList<CanvasComponentContainer> NetList;
        typedef NetworkingObjects::NetObjOwnerPtr<NetList> NetListOwnerPtr;
        typedef NetworkingObjects::NetObjTemporaryPtr<NetList> NetListTemporaryPtr;
        typedef NetworkingObjects::NetObjOrderedListObjectInfo<CanvasComponentContainer> ObjInfo;
        typedef NetworkingObjects::NetObjOrderedListIterator<CanvasComponentContainer> ObjInfoIterator;

        constexpr static int COMP_MAX_SHIFT_BEFORE_STOP_COLLISIONS = 14;
        constexpr static int COMP_MAX_SHIFT_BEFORE_STOP_SCALING = 14;
        constexpr static float COMP_MAX_BEFORE_STOP_SCALING = 1 << COMP_MAX_SHIFT_BEFORE_STOP_SCALING;
        constexpr static int COMP_MIN_SHIFT_BEFORE_DISAPPEAR = 11;
        constexpr static int COMP_COLLIDE_MIN_SHIFT_TINY = 9;
        constexpr static int COMP_MIPMAP_LEVEL_ONE = 2;
        constexpr static int COMP_MIPMAP_LEVEL_TWO = 5;

        struct CopyData {
            CoordSpaceHelper coords;
            std::unique_ptr<CanvasComponent> obj;
            void scale_up(const WorldScalar& scaleUpAmount);
        };

        struct TransformData {
            Vector2f translation;
            float rotation;
            float scale;
        };
        struct PreDrawData {
            TransformData transformData;
            std::shared_ptr<void> extraData;
        };
        std::optional<PreDrawData> preDrawDataHolder; // Can be used to store transforms when calculating transforms in parallel (and other predraw data)
        
        CanvasComponentContainer();
        CanvasComponentContainer(NetworkingObjects::NetObjManager& objMan, CanvasComponentType type);
        CanvasComponentContainer(NetworkingObjects::NetObjManager& objMan, const CopyData& copyData);

        static void register_class(World& w);
        void reassign_netobj_ids_call();
        std::unique_ptr<CopyData> get_data_copy() const;
        void save_file(cereal::PortableBinaryOutputArchive& a) const;
        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version, NetworkingObjects::NetObjManager& objMan);
        CanvasComponent& get_comp() const;
        const std::optional<SCollision::AABB<WorldScalar>>& get_world_bounds() const;
        void draw(SkCanvas* canvas, const DrawData& drawData) const;
        void draw_with_predraw_data(SkCanvas* canvas, const DrawData& drawData, const PreDrawData& preDrawData) const;
        PreDrawData calculate_predraw_data(const DrawData& drawData) const;
        static TransformData calculate_draw_transform(const CoordSpaceHelper& camCoords, const CoordSpaceHelper& coords);
        static void canvas_do_transform(SkCanvas* canvas, const TransformData& transformData);
        void commit_update(DrawingProgram& drawP);
        void commit_transform_dont_invalidate_cache(); // Must be thread safe
        void commit_transform(DrawingProgram& drawP);
        void commit_update_dont_invalidate_cache(DrawingProgram& drawP); // Must be thread safe
        bool should_draw(const DrawData& drawData) const;
        CanvasComponentEraseDetailResult collides_with_erase_detail(const CoordSpaceHelper& camCoords, const SkPath& checkAgainstCam) const;
        bool collides_with(const CoordSpaceHelper& camCoords, const SkPath& checkAgainstCam) const;
        bool collides_with_point(const CoordSpaceHelper& camCoords, const Vector2f& checkAgainstCam) const;
        void send_comp_update(DrawingProgram& drawP, bool finalUpdate);
        void scale_up(const WorldScalar& scaleUpAmount);

        std::weak_ptr<DrawingProgramCacheBVHNode> cacheParentBvhNode;
        DrawingProgramLayerListItem* parentLayer = nullptr;
        CoordSpaceHelper coords;
        ObjInfoIterator objInfo;
    private:
        friend class BrushStrokeCanvasComponent;
        friend class ImageCanvasComponent;

        static void write_constructor_func(const NetworkingObjects::NetObjTemporaryPtr<CanvasComponentContainer>& o, cereal::PortableBinaryOutputArchive& a);

        unsigned get_mipmap_level(const DrawData& drawData) const;
        CanvasComponent* allocate_comp(CanvasComponentType type);
        void calculate_world_bounds();

        std::optional<SCollision::AABB<WorldScalar>> worldAABB;
        NetworkingObjects::NetObjOwnerPtr<CanvasComponentAllocator> compAllocator;
};
