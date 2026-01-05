#include "World.hpp"
#include <Helpers/HsvRgb.hpp>
#include <Helpers/MathExtras.hpp>
#include <Helpers/Networking/ByteStream.hpp>
#include <Helpers/NetworkingObjects/NetObjManager.hpp>
#include <Helpers/NetworkingObjects/NetObjManagerTypeList.hpp>
#include <Helpers/VersionNumber.hpp>
#include <Helpers/NetworkingObjects/NetObjGenericSerializedClass.hpp>
#include <Helpers/NetworkingObjects/DelayUpdateSerializedClassManager.hpp>
#include <cereal/types/unordered_map.hpp>
#include "DrawingProgram/Layers/DrawingProgramLayerListItem.hpp"
#include "Helpers/NetworkingObjects/NetObjOrderedList.hpp"
#include "Helpers/NetworkingObjects/NetObjTemporaryPtr.decl.hpp"
#include "Helpers/NetworkingObjects/NetObjUnorderedSet.hpp"
#include "CommandList.hpp"
#include "MainProgram.hpp"
#include "SharedTypes.hpp"
#include "Toolbar.hpp"
#include "VersionConstants.hpp"
#include "WorldGrid.hpp"
#include <cereal/archives/portable_binary.hpp>
#include <chrono>
#include <fstream>
#include "FileHelpers.hpp"
#include "FontData.hpp"
#include <include/core/SkFontMetrics.h>
#include <Helpers/Networking/NetLibrary.hpp>
#include <Helpers/Logger.hpp>
#include <cereal/types/vector.hpp>
#include <Helpers/Networking/NetLibrary.hpp>
#include <zstd.h>
#include "CanvasComponents/CanvasComponent.hpp"
#include "CanvasComponents/CanvasComponentContainer.hpp"
#include "CanvasComponents/CanvasComponentAllocator.hpp"

#ifdef __EMSCRIPTEN__
    #include <EmscriptenHelpers/emscripten_browser_file.h>
#endif

World::World(MainProgram& initMain, OpenWorldInfo& worldInfo):
    netObjMan(!worldInfo.isClient),
    main(initMain),
    undo(*this),
    rMan(*this),
    drawProg(*this),
    bMan(*this),
    gridMan(*this),
    canvasTheme(*this)
{
    init_net_obj_type_list();
    netObjMan.set_netobj_destroy_callback([&undo = undo](const NetworkingObjects::NetObjID& idToDestroy) {
        undo.remove_by_netid(idToDestroy);
    });
    netObjMan.set_netid_reassign_callback([&undo = undo](const NetworkingObjects::NetObjID& oldID, const NetworkingObjects::NetObjID& newID) {
        undo.reassign_netid(oldID, newID);
    });

    netSource = worldInfo.netSource;

    drawData.rMan = &rMan;
    drawData.main = &main;

    if(worldInfo.isClient)
        init_client(worldInfo.netSource);
    else {
        init_client_data_list();
        if(worldInfo.filePathSource.has_value())
            load_from_file(worldInfo.filePathSource.value(), worldInfo.fileDataBuffer);
        else
            load_empty_canvas();
        set_name(name);
        #ifdef ENABLE_ORDERED_LIST_TEST
            listDebugTest = netObjMan.make_obj<NetworkingObjects::NetObjOrderedList<uint16_t>>();
        #endif
    }
}

void World::init_client_data_list() {
    clients = netObjMan.make_obj<NetworkingObjects::NetObjUnorderedSet<ClientData>>();
    ownClientData = clients->emplace_direct(clients, ClientData::InitStruct{
        .cursorColor = get_random_cursor_color(),
        .displayName = main.displayName
    });
    init_client_data_list_callbacks();
}

Vector3f World::get_random_cursor_color() {
    Vector3f hsv;
    hsv[0] = Random::get().real_range(0.0f, 360.0f);
    hsv[1] = Random::get().real_range(0.3f, 0.7f);
    hsv[2] = 1.0;
    return hsv_to_rgb<Vector3f>(hsv);
}

void World::init_client_data_list_callbacks() {
    clients->set_insert_callback([&](const NetworkingObjects::NetObjOwnerPtr<ClientData>& objPtr) {
        add_chat_message(objPtr->get_display_name(), "joined", Toolbar::ChatMessage::Type::JOIN);
    });
    clients->set_erase_callback([&](const NetworkingObjects::NetObjOwnerPtr<ClientData>& objPtr) {
        add_chat_message(objPtr->get_display_name(), "left", Toolbar::ChatMessage::Type::JOIN);
    });
}

void World::init_net_obj_type_list() {
    BookmarkListItem::register_class(*this);
    WorldGrid::register_class(*this);
    NetworkingObjects::register_ordered_list_class<WorldGrid>(netObjMan);
    CanvasComponentAllocator::register_class(*this);
    CanvasComponentContainer::register_class(*this);
    NetworkingObjects::register_ordered_list_class<CanvasComponentContainer>(netObjMan);
    DrawingProgramLayerListItem::register_class(*this);
    NetworkingObjects::register_ordered_list_class<DrawingProgramLayerListItem>(netObjMan);
    canvasTheme.register_class();
    ClientData::register_class(*this);
    NetworkingObjects::register_unordered_set_class<ClientData>(netObjMan);

#ifdef ENABLE_ORDERED_LIST_TEST
    NetworkingObjects::register_generic_serialized_class<uint16_t>(netObjMan);
    NetworkingObjects::register_ordered_list_class<uint16_t>(netObjMan);
#endif
}

void World::init_client(const std::string& serverFullID) {
    main.init_net_library();
    clientStillConnecting = true;
    netClient = std::make_shared<NetClient>(serverFullID);
    lastKeepAliveSent = std::chrono::steady_clock::now();
    NetLibrary::register_client(netClient);
    netObjMan.set_client(netClient, SERVER_UPDATE_NETWORK_OBJECT, SERVER_UPDATE_MANY_NETWORK_OBJECTS);
    set_name("");

    rMan.init_client_callbacks();
    drawProg.init_client_callbacks();
    netClient->add_recv_callback(CLIENT_INITIAL_DATA, [&](cereal::PortableBinaryInputArchive& message) {
        std::string fileDisplayName;
        NetworkingObjects::NetObjID clientDataObjID;
        message(fileDisplayName, clientDataObjID);
        set_name(fileDisplayName);
        bMan.bookmarkListRoot = netObjMan.read_create_message<BookmarkListItem>(message, nullptr);
        gridMan.grids = netObjMan.read_create_message<NetworkingObjects::NetObjOrderedList<WorldGrid>>(message, nullptr);
        drawProg.read_components_client(message);
        canvasTheme.read_create_message(message);
        clients = netObjMan.read_create_message<NetworkingObjects::NetObjUnorderedSet<ClientData>>(message, nullptr);
        init_client_data_list_callbacks();
        ownClientData = netObjMan.get_obj_temporary_ref_from_id<ClientData>(clientDataObjID);
        drawData.cam.smooth_move_to(*main.world, ownClientData->get_cam_coords(), ownClientData->get_window_size(), true);

        #ifdef ENABLE_ORDERED_LIST_TEST
            listDebugTest = netObjMan.read_create_message<NetworkingObjects::NetObjOrderedList<uint16_t>>(message, nullptr);
        #endif

        clientStillConnecting = false;
    });
    netClient->add_recv_callback(CLIENT_UPDATE_NETWORK_OBJECT, [&](cereal::PortableBinaryInputArchive& message) {
        netObjMan.read_update_message(message, nullptr);
    });
    netClient->add_recv_callback(CLIENT_UPDATE_MANY_NETWORK_OBJECTS, [&](cereal::PortableBinaryInputArchive& message) {
        netObjMan.read_many_update_message(message, nullptr);
    });
    netClient->add_recv_callback(CLIENT_KEEP_ALIVE, [&](cereal::PortableBinaryInputArchive& message) {
    });

    netClient->send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_INITIAL_DATA, main.displayName);
}

void World::focus_update() {
    connection_update();
    if(setToDestroy)
        return;

    if(!clientStillConnecting) {
        delayedUpdateObjectManager.update(netObjMan);
        constexpr float SECONDS_TO_SEND_CAMERA_DATA = 0.5f;
        timeToSendCameraData.update_time_since();
        if(timeToSendCameraData.get_time_since() > SECONDS_TO_SEND_CAMERA_DATA) {
            timeToSendCameraData.update_time_point();
            ownClientData->set_window_size(ownClientData, main.window.size.cast<float>().eval());
            ownClientData->set_camera_coords(ownClientData, drawData.cam.c);
        }
        ownClientData->set_cursor_pos(ownClientData, main.input.mouse.pos);
        drawProg.update();
        #ifdef ENABLE_ORDERED_LIST_TEST
            list_debug_test_update();
        #endif
    }

    drawData.cam.update_main(*this);

    rMan.update();

    mousePreviousWorldVec = drawData.cam.c.from_space(main.input.mouse.pos);
    mouseWorldMove = drawData.cam.c.from_space(main.input.mouse.move);

    if(main.input.key(InputManager::KEY_UNDO).repeat)
        undo_with_checks();
    else if(main.input.key(InputManager::KEY_REDO).repeat)
        redo_with_checks();
}

void World::connection_update() {
    if(netServer) {
        if(netServer->is_disconnected()) {
            Logger::get().log("USERINFO", "Host connection failed");
            netSource.clear();
            netServer = nullptr;
            netObjMan.disconnect();
        }
        else {
            netServer->update();
            if(std::chrono::steady_clock::now() - lastKeepAliveSent > std::chrono::seconds(2)) {
                netServer->send_items_to_all_clients(UNRELIABLE_COMMAND_CHANNEL, CLIENT_KEEP_ALIVE);
                lastKeepAliveSent = std::chrono::steady_clock::now();
            }
        }
    }
    else if(netClient) {
        if(netClient->is_disconnected()) {
            Logger::get().log("USERINFO", "Client connection failed");
            setToDestroy = true;
            netObjMan.disconnect();
        }
        netClient->update();
        if(std::chrono::steady_clock::now() - lastKeepAliveSent > std::chrono::seconds(2)) {
            netClient->send_items_to_server(UNRELIABLE_COMMAND_CHANNEL, SERVER_KEEP_ALIVE);
            lastKeepAliveSent = std::chrono::steady_clock::now();
        }
    }
}

void World::undo_with_checks() {
    if(!clientStillConnecting && !drawProg.prevent_undo_or_redo())
        undo.undo();
}

void World::redo_with_checks() {
    if(!clientStillConnecting && !drawProg.prevent_undo_or_redo())
        undo.redo();
}

void World::unfocus_update() {
    connection_update();
    if(setToDestroy)
        return;
}

void World::send_chat_message(const std::string& message) {
    if(!clientStillConnecting)
        ownClientData->send_chat_message(ownClientData, *this, message);
}

void World::add_chat_message(const std::string& name, const std::string& message, Toolbar::ChatMessage::Type type) {
    Logger::get().log("CHAT", type == Toolbar::ChatMessage::JOIN ? (name + " " + message) : ("[" + name + "] " + message));
    chatMessages.emplace_front(Toolbar::ChatMessage{name, message, type});
    if(chatMessages.size() == CHAT_SIZE)
        chatMessages.pop_back();
}

WorldVec World::get_mouse_world_pos() {
    return mousePreviousWorldVec;
}

WorldVec World::get_mouse_world_move() {
    return mouseWorldMove;
}

void World::early_destroy() {
    //con.early_destroy();
}

void World::set_name(const std::string& n) {
    if(n.empty())
        name = "New File";
    else
        name = n;
}

void World::ensure_display_name_unique(std::string& displayName) {
    for(;;) {
        bool isUnique = true;
        for(auto& client : clients->get_data()) {
            if(client->get_display_name() == displayName) {
                size_t leftParenthesisIndex = displayName.find_last_of('(');
                size_t rightParenthesisIndex = displayName.find_last_of(')');
                bool failToIncrement = true;
                if(leftParenthesisIndex != std::string::npos && rightParenthesisIndex != std::string::npos && leftParenthesisIndex < rightParenthesisIndex) {
                    std::string numStr = displayName.substr(leftParenthesisIndex + 1, rightParenthesisIndex - leftParenthesisIndex - 1);
                    bool isAllDigits = true;
                    for(char c : numStr) {
                        if(!isdigit(c)) {
                            isAllDigits = false;
                            break;
                        }
                    }
                    if(isAllDigits) {
                        try {
                            int s = std::stoi(numStr);
                            s++;
                            displayName = displayName.substr(0, leftParenthesisIndex);
                            displayName += "(" + std::to_string(s) + ")";
                            failToIncrement = false;
                        }
                        catch(...) {}
                    }
                }
                if(failToIncrement)
                    displayName += " (2)";
                isUnique = false;
                break;
            }
        }
        if(isUnique)
            break;
    }
}

void World::start_hosting(const std::string& initNetSource, const std::string& serverLocalID) {
    main.init_net_library();
    netServer = std::make_shared<NetServer>(serverLocalID);
    lastKeepAliveSent = std::chrono::steady_clock::now();
    NetLibrary::register_server(netServer);
    netObjMan.set_server(netServer, CLIENT_UPDATE_NETWORK_OBJECT, CLIENT_UPDATE_MANY_NETWORK_OBJECTS);
    netSource = initNetSource;
    rMan.init_server_callbacks();
    drawProg.init_server_callbacks();
    netServer->add_recv_callback(SERVER_INITIAL_DATA, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        ClientData::InitStruct newClientData;
        newClientData.cursorColor = get_random_cursor_color();
        message(newClientData.displayName);
        ensure_display_name_unique(newClientData.displayName);

        newClientData.camCoords = ownClientData->get_cam_coords();
        newClientData.windowSize = ownClientData->get_window_size();
        newClientData.gridSize = ownClientData->get_grid_size();

        NetworkingObjects::NetObjTemporaryPtr<ClientData> clientDataObjPtr = clients->emplace_direct(clients, newClientData);
        client->customID = clientDataObjPtr.get_net_id().data;
        auto ss(std::make_shared<std::stringstream>());
        {
            cereal::PortableBinaryOutputArchive a(*ss);
            a(CLIENT_INITIAL_DATA, name, clientDataObjPtr.get_net_id());
            bMan.bookmarkListRoot.write_create_message(a);
            gridMan.grids.write_create_message(a);
            drawProg.write_components_server(a);
            canvasTheme.write_create_message(a);
            clients.write_create_message(a);
            #ifdef ENABLE_ORDERED_LIST_TEST
                listDebugTest.write_create_message(a);
            #endif
        }
        netServer->send_string_stream_to_client(client, RELIABLE_COMMAND_CHANNEL, ss);
        for(auto& r : rMan.resource_list()) {
            netServer->send_items_to_client(client, RESOURCE_COMMAND_CHANNEL, CLIENT_NEW_RESOURCE_ID, r.get_net_id());
            netServer->send_items_to_client(client, RESOURCE_COMMAND_CHANNEL, CLIENT_NEW_RESOURCE_DATA, *r);
        }
    });
    netServer->add_recv_callback(SERVER_UPDATE_NETWORK_OBJECT, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        netObjMan.read_update_message(message, client);
    });
    netServer->add_recv_callback(SERVER_UPDATE_MANY_NETWORK_OBJECTS, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
        netObjMan.read_many_update_message(message, client);
    });
    netServer->add_recv_callback(SERVER_KEEP_ALIVE, [&](std::shared_ptr<NetServer::ClientData> client, cereal::PortableBinaryInputArchive& message) {
    });
    netServer->add_disconnect_callback([&](std::shared_ptr<NetServer::ClientData> client) {
        NetworkingObjects::NetObjID idToErase;
        idToErase.data = client->customID;
        clients->erase(clients, idToErase);
    });
}

void World::save_to_file(const std::filesystem::path& filePathToSaveAt) {
    try {
        filePath = force_extension_on_path(filePathToSaveAt, FILE_EXTENSION);

        std::stringstream f;
        f.write(VersionConstants::CURRENT_SAVEFILE_HEADER.c_str(), VersionConstants::SAVEFILE_HEADER_LEN);

        {
            std::stringstream fWorldDataToCompress;
            {
                cereal::PortableBinaryOutputArchive a(fWorldDataToCompress);
                save_file(a);
            }

            std::vector<char> compressedData(ZSTD_compressBound(fWorldDataToCompress.view().size()));
            size_t trueCompressedSize = ZSTD_compress(compressedData.data(), compressedData.size(), fWorldDataToCompress.view().data(), fWorldDataToCompress.view().size(), ZSTD_CLEVEL_DEFAULT);
            std::string_view compressedF(compressedData.data(), trueCompressedSize);

            f << compressedF;
        }

        set_name(filePath.stem().string());

        #ifdef __EMSCRIPTEN__
            emscripten_browser_file::download(
                filePath.string(),
                "application/octet-stream",
                f.view()
            );
        #else
            std::ofstream fi(filePath, std::ios::out | std::ios::binary);
            fi << f.view();
            fi.close();
        #endif

        Logger::get().log("USERINFO", "File saved");
    }
    catch(const std::exception& e) {
        Logger::get().log("WORLDFATAL", std::string("Save error: ") + e.what());
    }
}

void World::load_empty_canvas() {
    gridMan.server_init_no_file();
    bMan.server_init_no_file();
    drawProg.server_init_no_file();
    canvasTheme.server_init_no_file();
}

void World::load_from_file(const std::filesystem::path& filePathToLoadFrom, std::string_view buffer) {
    filePath = force_extension_on_path(filePathToLoadFrom, FILE_EXTENSION);

    std::string byteDataFromFile;

    if(buffer.empty()) {
        byteDataFromFile = read_file_to_string(filePath);
        buffer = byteDataFromFile;
    }

    if(buffer.size() < VersionConstants::SAVEFILE_HEADER_LEN)
        throw std::runtime_error("[World::load_from_file] File is not an InfiniPaint canvas (File too small)");

    std::string_view fileHeader = buffer.substr(0, VersionConstants::SAVEFILE_HEADER_LEN);
    VersionNumber fileVersion = VersionConstants::header_to_version_number(std::string(fileHeader));
    std::string_view uncompressedDataView;
    std::vector<char> uncompressedDataVector;

    if(fileVersion < VersionNumber(0, 1, 0))
        uncompressedDataView = std::string_view(buffer.data() + VersionConstants::SAVEFILE_HEADER_LEN, buffer.size() - VersionConstants::SAVEFILE_HEADER_LEN);
    else {
        uncompressedDataVector.resize(ZSTD_getFrameContentSize(buffer.data() + VersionConstants::SAVEFILE_HEADER_LEN, buffer.size() - VersionConstants::SAVEFILE_HEADER_LEN));
        size_t trueUncompressedSize = ZSTD_decompress(uncompressedDataVector.data(), uncompressedDataVector.size(), buffer.data() + VersionConstants::SAVEFILE_HEADER_LEN, buffer.size() - VersionConstants::SAVEFILE_HEADER_LEN);
        uncompressedDataView = std::string_view(uncompressedDataVector.data(), trueUncompressedSize);
    }

    ByteMemStream f((char*)uncompressedDataView.data(), uncompressedDataView.size());

    Logger::get().log("INFO", "Loading file from version " + version_numbers_to_version_str(fileVersion));

    cereal::PortableBinaryInputArchive a(f);
    load_file(a, fileVersion);

    Logger::get().log("USERINFO", "File loaded");
    set_name(filePath.stem().string());
}

void World::save_file(cereal::PortableBinaryOutputArchive& a) const {
    drawData.cam.save_file(a, *this);
    canvasTheme.save_file(a);
    drawProg.save_file(a);
    bMan.save_file(a);
    gridMan.save_file(a);
    rMan.save_file(a);
}

void World::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    drawData.cam.load_file(a, version, *this);
    canvasTheme.load_file(a, version);
    drawProg.load_file(a, version);
    bMan.load_file(a, version);
    gridMan.load_file(a, version);
    rMan.load_file(a, version);
}

WorldScalar World::calculate_zoom_from_uniform_zoom(WorldScalar uniformZoom, WorldVec oldWindowSize) {
    WorldScalar a1(main.window.size.x() / static_cast<double>(oldWindowSize.x()));
    WorldScalar a2(main.window.size.y() / static_cast<double>(oldWindowSize.y()));
    return uniformZoom * ((a1 < a2) ? a1 : a2);
}

void World::draw_other_player_cursors(SkCanvas* canvas, const DrawData& drawData) {
    if(clients) {
        for(auto& clientDataPtr : clients->get_data()) {
            if(clientDataPtr != ownClientData)
                clientDataPtr->draw_cursor(canvas, drawData);
        }
    }
}

void World::scale_up_step() {
    if(ownClientData)
        ownClientData->scale_up_step(ownClientData, *this);
}

void World::scale_up(const WorldScalar& scaleUpAmount) {
    Logger::get().log("USERINFO", "Canvas scaled up");
    bMan.scale_up(scaleUpAmount);
    gridMan.scale_up(scaleUpAmount);
    drawData.cam.scale_up(*this, scaleUpAmount);
    // drawProg will be sending info on committed objects/modified grids. These objects will be scaled up already.
    // This means that the scale up message must be send BEFORE the World::scale_up function is called on our end
    // if this client is the one responsible for the scale up
    drawProg.scale_up(scaleUpAmount);
    undo.scale_up(scaleUpAmount);
}

#ifdef ENABLE_ORDERED_LIST_TEST
void World::list_debug_test_update() {
    if(std::chrono::steady_clock::now() < (listDebugTestTimeStart + std::chrono::minutes(1))) {
        bool isInsert;
        if(listDebugTest->size() > 70)
            isInsert = false;
        else if(listDebugTest->size() < 20)
            isInsert = true;
        else
            isInsert = Random::get().real_range(0.0f, 1.0f) > 0.5f;

        using namespace NetworkingObjects;
        if(isInsert) {
            std::vector<std::pair<NetObjOrderedListIterator<uint16_t>, NetObjOwnerPtr<uint16_t>>> toInsert;
            std::vector<uint32_t> randomIndices;
            uint32_t insertAmount = Random::get().int_range(5, 25);
            for(uint32_t i = 0; i < insertAmount; i++)
                randomIndices.emplace_back(Random::get().int_range<uint32_t>(0, listDebugTest->size() + 10));
            randomIndices[1] = randomIndices[0];
            std::sort(randomIndices.begin(), randomIndices.end());
            for(uint32_t index : randomIndices)
                toInsert.emplace_back(listDebugTest->at(index), netObjMan.make_obj_direct<uint16_t>(Random::get().int_range<uint32_t>(10, 100)));
            listDebugTest->insert_ordered_list_and_send_create(listDebugTest, toInsert);
        }
        else {
            std::vector<NetObjOrderedListIterator<uint16_t>> toErase;
            for(uint32_t i = 0; i < listDebugTest->size(); i++) {
                if(Random::get().real_range(0.0f, 1.0f) > 0.8f)
                    toErase.emplace_back(listDebugTest->at(i));
            }
            listDebugTest->erase_list(listDebugTest, toErase);
        }
    }
    if(netServer && netServer->get_client_list().empty())
        listDebugTestTimeStart = std::chrono::steady_clock::now();
}
#endif

void World::draw(SkCanvas* canvas) {
    drawData.refresh_draw_optimizing_values();
    if(drawData.drawGrids)
        gridMan.draw_back(canvas, drawData);
    drawProg.draw(canvas, drawData);
    if(drawData.drawGrids) {
        gridMan.draw_front(canvas, drawData);
        gridMan.draw_coordinates(canvas, drawData);
    }
    draw_other_player_cursors(canvas, drawData);
}
