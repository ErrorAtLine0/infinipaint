#include "ResourceManager.hpp"
#include "Helpers/Networking/NetLibrary.hpp"
#include <include/core/SkImage.h>
#include "ResourceDisplay/SvgResourceDisplay.hpp"
#include "Server/CommandList.hpp"
#include "ResourceDisplay/ImageResourceDisplay.hpp"
#include "ResourceDisplay/FileResourceDisplay.hpp"
#include <cereal/types/string.hpp>
#include <Helpers/Logger.hpp>
#include <optional>
#include <sstream>
#include <fstream>
#include "World.hpp"
#include "ResourceDisplay/FileResourceDisplay.hpp"

ResourceManager::ResourceManager(World& initWorld):
    world(initWorld) 
{}

void ResourceManager::init_client_callbacks() {
    world.con.client_add_recv_callback(CLIENT_NEW_RESOURCE_ID, [&](cereal::PortableBinaryInputArchive& message) {
        message(resourceBeingRetrieved);
    });
    world.con.client_add_recv_callback(CLIENT_NEW_RESOURCE_DATA, [&](cereal::PortableBinaryInputArchive& message) {
        message(resources[resourceBeingRetrieved]);
        Logger::get().log("INFO", "Received new resource with id: " + std::to_string(resourceBeingRetrieved.first) + " " + std::to_string(resourceBeingRetrieved.second) + " of size " + std::to_string(resources[resourceBeingRetrieved].data->size()));
        resourceBeingRetrieved = {0, 0};
    });
}

ServerClientID ResourceManager::get_resource_being_retrieved() {
    return resourceBeingRetrieved;
}

float ResourceManager::get_resource_retrieval_progress() {
    std::pair<uint64_t, uint64_t> progressBytes = world.con.client_get_resource_retrieval_progress();
    if(progressBytes.second == 0)
        return 0.0f;
    else
        return static_cast<float>(progressBytes.first) / static_cast<float>(progressBytes.second);
}

void ResourceManager::update() {
    for(auto& [k, v] : displays)
        v->update(world);
}

ServerClientID ResourceManager::add_resource_file(std::string_view fileName) {
    // https://nullptr.org/cpp-read-file-into-string/
    std::ifstream file(fileName.data(), std::ios::in | std::ios::binary | std::ios::ate);
    if(!file.is_open()) {
        Logger::get().log("INFO", "[ResourceManager::add_resource_file] Could not open file " + std::string(fileName));
        return {0, 0};
    }

    size_t resourcesize;

    auto tellgResult = file.tellg();

    if(tellgResult == -1) {
        Logger::get().log("INFO", "[ResourceManager::add_resource_file] tellg failed for file " + std::string(fileName));
        return {0, 0};
    }

    resourcesize = static_cast<size_t>(tellgResult);

    file.seekg(0, std::ios_base::beg);

    ResourceData resource;
    resource.data = std::make_shared<std::string>();
    resource.name = std::filesystem::path(fileName).filename().string();
    std::string& toPlaceIn = *resource.data;

    toPlaceIn.resize(resourcesize);

    file.read(toPlaceIn.data(), resourcesize);

    file.close();

    Logger::get().log("INFO", "[ResourceManager::add_resource_file] Successfully read file " + std::string(fileName));

    return add_resource(resource);
}

ServerClientID ResourceManager::add_resource(const ResourceData& resource) {
    auto it = std::find_if(resources.begin(), resources.end(), [&](const auto& p) {
        return p.second.data == resource.data || (*p.second.data) == (*resource.data);
    });
    if(it != resources.end()) {
        Logger::get().log("INFO", "[ResourceManager::add_resource] File " + std::string(resource.name) + " is a duplicate");
        return it->first;
    }
    ServerClientID newID = world.get_new_id();
    resources[newID] = resource;
    world.con.client_send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_RESOURCE_INIT, newID);
    world.con.client_send_items_to_server(RESOURCE_COMMAND_CHANNEL, SERVER_RESOURCE_DATA, resource);
    return newID;
}

void ResourceManager::copy_resource_set_to_map(const std::unordered_set<ServerClientID>& resourceSet, std::unordered_map<ServerClientID, ResourceData>& resourceMap) const {
    resourceMap.clear();
    for(auto& [k, v] : resources) {
        if(resourceSet.contains(k))
            resourceMap[k] = v;
    }
}

ClientPortionID ResourceManager::get_max_id(ServerPortionID serverID) const {
    ClientPortionID maxClientID = 0;
    for(auto& p : resources) {
        if(p.first.first == serverID)
            maxClientID = std::max(maxClientID, p.first.second);
    }
    return maxClientID;
}

std::optional<ResourceData> ResourceManager::get_resource(const ServerClientID& id) const {
    auto it = resources.find(id);
    if(it == resources.end())
        return std::nullopt;
    return it->second;
}

std::shared_ptr<ResourceDisplay> ResourceManager::get_display_data(const ServerClientID& fileID) {
    auto loadedDisplayIt = displays.find(fileID);
    if(loadedDisplayIt != displays.end())
        return loadedDisplayIt->second;

    auto resourceIt = resources.find(fileID);
    if(resourceIt == resources.end())
        return nullptr;

    auto imgResource(std::make_shared<ImageResourceDisplay>());
    if(imgResource->load(*this, resourceIt->second.name, *resourceIt->second.data)) {
        displays[fileID] = imgResource;
        return imgResource;
    }

    auto svgResource(std::make_shared<SvgResourceDisplay>());
    if(svgResource->load(*this, resourceIt->second.name, *resourceIt->second.data)) {
        displays[fileID] = svgResource;
        return svgResource;
    }

    auto fileResource(std::make_shared<FileResourceDisplay>());
    fileResource->load(*this, resourceIt->second.name, *resourceIt->second.data);
    displays[fileID] = fileResource;
    return fileResource;
}

const std::unordered_map<ServerClientID, ResourceData>& ResourceManager::resource_list() {
    return resources;
}
