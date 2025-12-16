#include "ResourceManager.hpp"
#include <Helpers/Networking/NetLibrary.hpp>
#include <include/core/SkImage.h>
#include <Helpers/NetworkingObjects/NetObjID.hpp>
#include "ResourceDisplay/SvgResourceDisplay.hpp"
#include "Server/CommandList.hpp"
#include "ResourceDisplay/ImageResourceDisplay.hpp"
#include "ResourceDisplay/FileResourceDisplay.hpp"
#include <cereal/types/string.hpp>
#include <Helpers/Logger.hpp>
#include <fstream>
#include "World.hpp"
#include "ResourceDisplay/FileResourceDisplay.hpp"
#include <cereal/archives/portable_binary.hpp>

ResourceManager::ResourceManager(World& initWorld):
    world(initWorld) 
{}

void ResourceManager::init_client_callbacks() {
    world.con.client_add_recv_callback(SERVER_NEW_RESOURCE_ID, [&](cereal::PortableBinaryInputArchive& message) {
        NetworkingObjects::NetObjID idBeingRetrieved;
        message(idBeingRetrieved);
        resourcesBeingRetrieved.emplace(idBeingRetrieved, 0);
    });
    world.con.client_add_recv_callback(SERVER_NEW_RESOURCE_DATA, [&](cereal::PortableBinaryInputArchive& message) {
        ResourceData newResource;
        message(newResource);
        resourceList.emplace_back(world.netObjMan.make_obj_direct_with_specific_id<ResourceData>(resourcesBeingRetrieved.begin()->first, newResource));
        resourcesBeingRetrieved.clear();
    });
}

void ResourceManager::init_server_callbacks() {
    world.con.localServer->netServer->add_recv_callback(CLIENT_NEW_RESOURCE_ID, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        NetworkingObjects::NetObjID idBeingRetrieved;
        message(idBeingRetrieved);
        resourcesBeingRetrieved.emplace(idBeingRetrieved, client->customID);
    });
    world.con.localServer->netServer->add_recv_callback(CLIENT_NEW_RESOURCE_DATA, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        ResourceData newResource;
        message(newResource);
        auto it = std::find_if(resourcesBeingRetrieved.begin(), resourcesBeingRetrieved.end(), [&client](auto& p) {
            return p.second == client->customID;
        });
        resourceList.emplace_back(world.netObjMan.make_obj_direct_with_specific_id<ResourceData>(it->first, newResource));
        world.con.localServer->netServer->send_items_to_all_clients_except(client, RESOURCE_COMMAND_CHANNEL, SERVER_NEW_RESOURCE_ID, resourceList.back().get_net_id());
        world.con.localServer->netServer->send_items_to_all_clients_except(client, RESOURCE_COMMAND_CHANNEL, SERVER_NEW_RESOURCE_DATA, newResource);
        resourcesBeingRetrieved.erase(resourceList.back().get_net_id());
    });
}

float ResourceManager::get_resource_retrieval_progress(const NetworkingObjects::NetObjID& id) {
    auto resourceClientID = resourcesBeingRetrieved.find(id);
    if(resourceClientID == resourcesBeingRetrieved.end())
        return 0.0f;
    if(world.con.localServer) {
        uint64_t clientIDToLookFor = resourceClientID->second;

        const std::vector<std::shared_ptr<NetServer::ClientData>>& clientListInNetServer = world.con.localServer->netServer->get_client_list();
        auto netClientIt = std::find_if(clientListInNetServer.begin(), clientListInNetServer.end(), [&clientIDToLookFor](auto& nC) {
            return nC->customID == clientIDToLookFor;
        });
        if(netClientIt == clientListInNetServer.end()) {
            resourcesBeingRetrieved.erase(resourceClientID); // Client disconnected, remove it from the list
            return 0.0f;
        }
        auto progressBytes = (*netClientIt)->get_progress_into_fragmented_message(RESOURCE_COMMAND_CHANNEL);
        if(progressBytes.totalBytes == 0)
            return 0.0f;
        return static_cast<float>(progressBytes.downloadedBytes) / static_cast<float>(progressBytes.totalBytes);
    }
    else if(world.con.client_exists() && !world.con.is_client_disconnected()) {
        NetLibrary::DownloadProgress progressBytes = world.con.client_get_resource_retrieval_progress();
        if(progressBytes.totalBytes == 0)
            return 0.0f;
        return static_cast<float>(progressBytes.downloadedBytes) / static_cast<float>(progressBytes.totalBytes);
    }
    return 0.0f;
}

void ResourceManager::update() {
    for(auto& [k, v] : displays)
        v->update(world);
}

NetworkingObjects::NetObjTemporaryPtr<ResourceData> ResourceManager::add_resource_file(const std::filesystem::path& filePath) {
    // https://nullptr.org/cpp-read-file-into-string/
    std::ifstream file(filePath, std::ios::in | std::ios::binary | std::ios::ate);
    if(!file.is_open()) {
        Logger::get().log("INFO", "[ResourceManager::add_resource_file] Could not open file " + filePath.string());
        return {};
    }

    size_t resourcesize;

    auto tellgResult = file.tellg();

    if(tellgResult == -1) {
        Logger::get().log("INFO", "[ResourceManager::add_resource_file] tellg failed for file " + filePath.string());
        return {};
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

const NetworkingObjects::NetObjOwnerPtr<ResourceData>& ResourceManager::add_resource(const ResourceData& resource) {
    auto it = std::find_if(resourceList.begin(), resourceList.end(), [&](const auto& p) {
        return p->data == resource.data || (*p->data) == (*resource.data);
    });
    if(it != resourceList.end()) {
        Logger::get().log("INFO", "[ResourceManager::add_resource] File " + std::string(resource.name) + " is a duplicate");
        return *it;
    }
    auto& resourceInsert = resourceList.emplace_back(world.netObjMan.make_obj_direct<ResourceData>(resource));
    if(world.con.localServer) {
        world.con.localServer->netServer->send_items_to_all_clients(RESOURCE_COMMAND_CHANNEL, SERVER_NEW_RESOURCE_ID, resourceInsert.get_net_id());
        world.con.localServer->netServer->send_items_to_all_clients(RESOURCE_COMMAND_CHANNEL, SERVER_NEW_RESOURCE_DATA, resource);
    }
    else {
        world.con.client_send_items_to_server(RESOURCE_COMMAND_CHANNEL, SERVER_NEW_RESOURCE_ID, resourceInsert.get_net_id());
        world.con.client_send_items_to_server(RESOURCE_COMMAND_CHANNEL, SERVER_NEW_RESOURCE_DATA, resource);
    }
    return resourceInsert;
}

ResourceDisplay* ResourceManager::get_display_data(const NetworkingObjects::NetObjID& fileID) {
    auto loadedDisplayIt = displays.find(fileID);
    if(loadedDisplayIt != displays.end())
        return loadedDisplayIt->second.get();

    NetworkingObjects::NetObjTemporaryPtr<ResourceData> resourceData = world.netObjMan.get_obj_temporary_ref_from_id<ResourceData>(fileID);
    if(resourceData == NetworkingObjects::NetObjTemporaryPtr<ResourceData>())
        return nullptr;

    auto imgResource(std::make_unique<ImageResourceDisplay>());
    if(imgResource->load(*this, resourceData->name, *resourceData->data)) {
        displays.emplace(fileID, std::move(imgResource));
        return displays[fileID].get();
    }

    auto svgResource(std::make_unique<SvgResourceDisplay>());
    if(svgResource->load(*this, resourceData->name, *resourceData->data)) {
        displays.emplace(fileID, std::move(svgResource));
        return displays[fileID].get();
    }

    auto fileResource(std::make_unique<FileResourceDisplay>());
    fileResource->load(*this, resourceData->name, *resourceData->data);
    displays.emplace(fileID, std::move(fileResource));
    return displays[fileID].get();
}

const std::vector<NetworkingObjects::NetObjOwnerPtr<ResourceData>>& ResourceManager::resource_list() {
    return resourceList;
}
