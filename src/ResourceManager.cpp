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
        message(resourceBeingRetrievedClient);
    });
    world.con.client_add_recv_callback(CLIENT_NEW_RESOURCE_DATA, [&](cereal::PortableBinaryInputArchive& message) {
        message(resources[resourceBeingRetrievedClient]);
        Logger::get().log("INFO", "Received new resource with id: " + std::to_string(resourceBeingRetrievedClient.first) + " " + std::to_string(resourceBeingRetrievedClient.second) + " of size " + std::to_string(resources[resourceBeingRetrievedClient].data->size()));
        resourceBeingRetrievedClient = {0, 0};
    });
}

float ResourceManager::get_resource_retrieval_progress(const ServerClientID& id) {
    if(id == ServerClientID{0, 0})
        return 0.0f;
    if(world.con.host_exists() && !world.con.is_host_disconnected()) {
        auto it = serverDownloadProgress.find(id);
        if(it == serverDownloadProgress.end())
            return 0.0f;
        else {
            auto& progressBytes = it->second;
            if(progressBytes.totalBytes == 0)
                return 0.0f;
            return static_cast<float>(progressBytes.downloadedBytes) / static_cast<float>(progressBytes.totalBytes);
        }
    }
    else if(world.con.client_exists() && !world.con.is_client_disconnected()) {
        if(id != resourceBeingRetrievedClient)
            return 0.0f;
        else {
            NetLibrary::DownloadProgress progressBytes = world.con.client_get_resource_retrieval_progress();
            if(progressBytes.totalBytes == 0)
                return 0.0f;
            return static_cast<float>(progressBytes.downloadedBytes) / static_cast<float>(progressBytes.totalBytes);
        }
    }
    return 0.0f;
}

void ResourceManager::update() {
    if(!world.con.host_exists() || world.con.is_host_disconnected())
        serverDownloadProgress.clear();
    else
        serverDownloadProgress = world.con.server_get_resource_retrieval_progress();

    for(auto& [k, v] : displays)
        v->update(world);
}

ServerClientID ResourceManager::add_resource_file(const std::filesystem::path& filePath) {
    // https://nullptr.org/cpp-read-file-into-string/
    std::ifstream file(filePath, std::ios::in | std::ios::binary | std::ios::ate);
    if(!file.is_open()) {
        Logger::get().log("INFO", "[ResourceManager::add_resource_file] Could not open file " + filePath.string());
        return {0, 0};
    }

    size_t resourcesize;

    auto tellgResult = file.tellg();

    if(tellgResult == -1) {
        Logger::get().log("INFO", "[ResourceManager::add_resource_file] tellg failed for file " + filePath.string());
        return {0, 0};
    }

    resourcesize = static_cast<size_t>(tellgResult);

    file.seekg(0, std::ios_base::beg);

    ResourceData resource;
    resource.data = std::make_shared<std::string>();
    resource.name = std::filesystem::path(filePath).filename().string();
    std::string& toPlaceIn = *resource.data;

    toPlaceIn.resize(resourcesize);

    file.read(toPlaceIn.data(), resourcesize);

    file.close();

    Logger::get().log("INFO", "[ResourceManager::add_resource_file] Successfully read file " + filePath.string());

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
    world.con.client_send_items_to_server(RESOURCE_COMMAND_CHANNEL, SERVER_NEW_RESOURCE_ID, newID);
    world.con.client_send_items_to_server(RESOURCE_COMMAND_CHANNEL, SERVER_NEW_RESOURCE_DATA, resource);
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

ResourceDisplay* ResourceManager::get_display_data(const ServerClientID& fileID) {
    auto loadedDisplayIt = displays.find(fileID);
    if(loadedDisplayIt != displays.end())
        return loadedDisplayIt->second.get();

    auto resourceIt = resources.find(fileID);
    if(resourceIt == resources.end())
        return nullptr;

    auto imgResource(std::make_unique<ImageResourceDisplay>());
    if(imgResource->load(*this, resourceIt->second.name, *resourceIt->second.data)) {
        displays.emplace(fileID, std::move(imgResource));
        return displays[fileID].get();
    }

    auto svgResource(std::make_unique<SvgResourceDisplay>());
    if(svgResource->load(*this, resourceIt->second.name, *resourceIt->second.data)) {
        displays.emplace(fileID, std::move(svgResource));
        return displays[fileID].get();
    }

    auto fileResource(std::make_unique<FileResourceDisplay>());
    fileResource->load(*this, resourceIt->second.name, *resourceIt->second.data);
    displays.emplace(fileID, std::move(fileResource));
    return displays[fileID].get();
}

const std::unordered_map<ServerClientID, ResourceData>& ResourceManager::resource_list() {
    return resources;
}
