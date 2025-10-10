#pragma once
#include "SharedTypes.hpp"
#include <include/codec/SkCodec.h>
#include <include/core/SkImage.h>
#include "ResourceDisplay/ResourceDisplay.hpp"
#include <cereal/types/utility.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/unordered_map.hpp>
#include <queue>
#include <Helpers/Networking/NetLibrary.hpp>

class World;

struct ResourceData {
    std::string name;
    std::shared_ptr<std::string> data;
    template <typename Archive> void save(Archive& a) const {
        a(name, *data);
    }
    template <typename Archive> void load(Archive& a) {
        data = std::make_shared<std::string>();
        a(name, *data);
    }
};

class ResourceManager {
    public:
        ResourceManager(World& initWorld);
        template <typename Archive> void serialize(Archive& a) {
            a(resources);
        }
        template <typename Archive> void save_strip_unused_resources(Archive& a, const std::unordered_set<ServerClientID>& usedResources) const {
            std::unordered_map<ServerClientID, ResourceData> strippedResources = resources;
            std::erase_if(strippedResources, [&](const auto& p) {
                return !usedResources.contains(p.first);
            });
            a(strippedResources);
        }
        void update();
        void copy_resource_set_to_map(const std::unordered_set<ServerClientID>& resourceSet, std::unordered_map<ServerClientID, ResourceData>& resourceMap) const;
        void init_client_callbacks();
        ServerClientID add_resource_file(std::string_view fileName);
        ServerClientID add_resource(const ResourceData& resource);
        ResourceDisplay* get_display_data(const ServerClientID& fileID);
        std::optional<ResourceData> get_resource(const ServerClientID& id) const;
        ClientPortionID get_max_id(ServerPortionID serverID) const;
        const std::unordered_map<ServerClientID, ResourceData>& resource_list();
        float get_resource_retrieval_progress(const ServerClientID& id);

        World& world;
    private:
        std::unordered_map<ServerClientID, NetLibrary::DownloadProgress> serverDownloadProgress;
        ServerClientID resourceBeingRetrievedClient = {0, 0};

        std::unordered_map<ServerClientID, ResourceData> resources;
        std::unordered_map<ServerClientID, std::unique_ptr<ResourceDisplay>> displays;

        static std::array<const SkCodecs::Decoder, 5> decoders;
};

