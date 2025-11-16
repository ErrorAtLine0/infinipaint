#pragma once
#include "Helpers/SCollision.hpp"
#include <Helpers/Hashes.hpp>
#include "cereal/archives/portable_binary.hpp"
#include "../SharedTypes.hpp"
#include "../CoordSpaceHelper.hpp"
#include "../Server/MainServer.hpp"
#include "../CollabList.hpp"
#include <Helpers/VersionNumber.hpp>

#ifndef IS_SERVER
#include "../DrawData.hpp"
#include <chrono>
#include <include/core/SkCanvas.h>
#include <include/core/SkVertices.h>
#endif

#define CLIENT_DRAWCOMP_DELAY_TIMER_DURATION std::chrono::seconds(6)

#define DRAWCOMP_MAX_SHIFT_BEFORE_STOP_COLLISIONS 14
#define DRAWCOMP_MAX_SHIFT_BEFORE_STOP_SCALING 14
#define DRAWCOMP_MIN_SHIFT_BEFORE_DISAPPEAR 11
#define DRAWCOMP_COLLIDE_MIN_SHIFT_TINY 9
#define DRAWCOMP_MIPMAP_LEVEL_ONE 2
#define DRAWCOMP_MIPMAP_LEVEL_TWO 5

class DrawingProgram;
class DrawComponent;
class DrawingProgramCacheBVHNode;

typedef CollabList<std::shared_ptr<DrawComponent>, ServerClientID> CollabListType;

void save(cereal::PortableBinaryOutputArchive& a, const CollabListType::ObjectInfo& o);
void load(cereal::PortableBinaryInputArchive& a, CollabListType::ObjectInfo& o);

struct SendOrderedComponentVectorOp {
    std::vector<CollabListType::ObjectInfoPtr>* v;
    void save(cereal::PortableBinaryOutputArchive& a) const;
    void load(cereal::PortableBinaryInputArchive& a);
};

class DrawComponent {
    public:
        static std::shared_ptr<DrawComponent> allocate_comp_type(DrawComponentType type);
        virtual void save(cereal::PortableBinaryOutputArchive& a) const = 0;
        virtual void load(cereal::PortableBinaryInputArchive& a) = 0;
        virtual void save_file(cereal::PortableBinaryOutputArchive& a) const = 0;
        virtual void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) = 0;
        virtual DrawComponentType get_type() const = 0;
        virtual void get_used_resources(std::unordered_set<ServerClientID>& v) const;
        virtual void remap_resource_ids(std::unordered_map<ServerClientID, ServerClientID>& newIDs);
        virtual ~DrawComponent(); 

        // NOTE: Permanantly altering the coordinates during transformation can lead to loss of detail for smaller objects
        CoordSpaceHelper coords;

        struct DrawSetupData {
            struct TransformDrawData {
                Vector2f translation;
                float rotation;
                float scale;
            } transformData;
            bool shouldDraw = false;
            uint8_t mipmapLevel = 0;
        } drawSetupData;

        void canvas_do_calculated_transform(SkCanvas* canvas);
        void calculate_draw_transform(const DrawData& drawData);

        void server_send_place(MainServer& server, uint64_t placement);

        void scale_up(const WorldScalar& scaleUpAmount);

        static void server_send_place_many(MainServer& server, std::vector<CollabListType::ObjectInfoPtr>& comps);
        static void server_send_erase(MainServer& server, ServerClientID id);
        static void server_send_erase_set(MainServer& server, const std::unordered_set<ServerClientID>& ids);
        void server_send_update(MainServer& server, bool final);
        static void server_send_transform_many(MainServer& server, const std::vector<std::pair<ServerClientID, CoordSpaceHelper>>& transforms);

        std::weak_ptr<CollabListType::ObjectInfo> collabListInfo;
        std::weak_ptr<DrawingProgramCacheBVHNode> parentBvhNode;
        ServerClientID id = {0, 0};

#ifndef IS_SERVER
        std::chrono::time_point<std::chrono::steady_clock> lastUpdateTime;

        bool bounds_draw_check(const DrawData& drawData) const;

        bool check_timers(DrawingProgram& drawP, const std::shared_ptr<DrawComponent>& delayedUpdatePtr);
        static void client_send_place_many(DrawingProgram& drawP, std::vector<CollabListType::ObjectInfoPtr>& comps);
        void client_send_place(DrawingProgram& drawP);
        void client_send_erase(DrawingProgram& drawP);
        static void client_send_erase_set(DrawingProgram& drawP, const std::unordered_set<ServerClientID>& ids);
        void client_send_update(DrawingProgram& drawP, bool final);
        static void client_send_transform_many(DrawingProgram& drawP, const std::vector<std::pair<ServerClientID, CoordSpaceHelper>>& transforms);

        std::optional<SCollision::AABB<WorldScalar>> worldAABB;

        std::optional<SCollision::AABB<WorldScalar>> get_world_bounds() const;
        void calculate_world_bounds();
        virtual SCollision::AABB<float> get_obj_coord_bounds() const = 0;

        virtual std::shared_ptr<DrawComponent> copy(DrawingProgram& drawP) const = 0;
        virtual std::shared_ptr<DrawComponent> deep_copy(DrawingProgram& drawP) const = 0;

        virtual void draw(SkCanvas* canvas, const DrawData& drawData) = 0;
        void commit_update(DrawingProgram& drawP, bool invalidateCache = true); // invalidateCache = false version should be thread safe
        void commit_transform(DrawingProgram& drawP, bool invalidateCache = true); // invalidateCache = false version should be thread safe
        virtual void initialize_draw_data(DrawingProgram& drawP) = 0;
        virtual void update(DrawingProgram& drawP) = 0;
        virtual bool collides_within_coords(const SCollision::ColliderCollection<float>& checkAgainst) = 0;
        bool collides_with_world_coords(const CoordSpaceHelper& camCoords, const SCollision::ColliderCollection<WorldScalar>& checkAgainstWorld);
        bool collides_with_cam_coords(const CoordSpaceHelper& camCoords, const SCollision::ColliderCollection<float>& checkAgainstCam);
        bool collides_with(const CoordSpaceHelper& camCoords, const SCollision::ColliderCollection<WorldScalar>& checkAgainstWorld, const SCollision::ColliderCollection<float>& checkAgainstCam);
        virtual void update_from_delayed_ptr(const std::shared_ptr<DrawComponent>& delayedUpdatePtr) = 0;
#endif
};
