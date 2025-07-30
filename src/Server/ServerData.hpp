#pragma once
#include "cereal/archives/portable_binary.hpp"
#include "../SharedTypes.hpp"
#include "../BookmarkManager.hpp"
#include "../ResourceManager.hpp"

class DrawComponent;

class ServerData {
    public:
        std::vector<std::pair<ServerClientID, std::shared_ptr<DrawComponent>>> components;
        std::unordered_map<ServerClientID, std::shared_ptr<DrawComponent>> idToComponentMap;
        std::unordered_map<ServerClientID, ResourceData> resources;
        std::unordered_map<ServerClientID, std::chrono::time_point<std::chrono::steady_clock>> updateComponentTimers;
        std::unordered_map<std::string, Bookmark> bookmarks;
        void save(cereal::PortableBinaryOutputArchive& a) const;
        void write_to_file(cereal::PortableBinaryOutputArchive& a) const;
        void load(cereal::PortableBinaryInputArchive& a);
        ClientPortionID get_max_id(ServerPortionID serverID) const;
};
