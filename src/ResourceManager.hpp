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
#include <Helpers/NetworkingObjects/NetObjOwnerPtr.hpp>
#include <include/codec/SkCodec.h>
#include "Helpers/NetworkingObjects/NetObjID.hpp"
#include "ResourceDisplay/ResourceDisplay.hpp"
#include <Helpers/Networking/NetLibrary.hpp>
#include <Helpers/NetworkingObjects/NetObjUnorderedSet.hpp>
#include <Helpers/VersionNumber.hpp>

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
        void update();
        void init_client_callbacks();
        void init_server_callbacks();
        NetworkingObjects::NetObjTemporaryPtr<ResourceData> add_resource_file(const std::filesystem::path& filePath);
        const NetworkingObjects::NetObjOwnerPtr<ResourceData>& add_resource(const ResourceData& resource);
        ResourceDisplay* get_display_data(const NetworkingObjects::NetObjID& fileID);
        const std::vector<NetworkingObjects::NetObjOwnerPtr<ResourceData>>& resource_list();
        float get_resource_retrieval_progress(const NetworkingObjects::NetObjID& id);
        std::unordered_map<NetworkingObjects::NetObjID, ResourceData> copy_resource_set_to_map(const std::unordered_set<NetworkingObjects::NetObjID>& resourceSet) const;
        void clear_display_cache();

        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version);
        void save_file(cereal::PortableBinaryOutputArchive& a) const;

        World& world;
    private:
        std::unordered_map<NetworkingObjects::NetObjID, std::unique_ptr<ResourceDisplay>> displays;

        std::unordered_map<NetworkingObjects::NetObjID, std::weak_ptr<NetServer::ClientData>> resourcesBeingRetrieved;
        std::vector<NetworkingObjects::NetObjOwnerPtr<ResourceData>> resourceList;

        static std::array<const SkCodecs::Decoder, 5> decoders;
};

