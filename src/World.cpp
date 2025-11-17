#include "World.hpp"
#include "DrawComponents/DrawComponent.hpp"
#include "Helpers/HsvRgb.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/Networking/ByteStream.hpp"
#include "Helpers/VersionNumber.hpp"
#include "Server/CommandList.hpp"
#include "MainProgram.hpp"
#include "SharedTypes.hpp"
#include "Toolbar.hpp"
#include "VersionConstants.hpp"
#include "WorldGrid.hpp"
#include "cereal/archives/portable_binary.hpp"
#include <fstream>
#include "FileHelpers.hpp"
#include "FontData.hpp"
#include <include/core/SkFontMetrics.h>
#include <Helpers/Networking/NetLibrary.hpp>
#include <Helpers/Logger.hpp>
#include <cereal/types/vector.hpp>
#include <Helpers/Networking/NetLibrary.hpp>
#include <zstd.h>

#ifdef __EMSCRIPTEN__
    #include <EmscriptenHelpers/emscripten_browser_file.h>
#endif

World::World(MainProgram& initMain, OpenWorldInfo& worldInfo):
    main(initMain),
    rMan(*this),
    drawProg(*this),
    bMan(*this),
    gridMan(*this)
{
    set_canvas_background_color(main.defaultCanvasBackgroundColor);
    displayName = main.displayName;

    netSource = worldInfo.netSource;

    if(worldInfo.conType == CONNECTIONTYPE_CLIENT || worldInfo.conType == CONNECTIONTYPE_SERVER)
        main.init_net_library();

    drawData.rMan = &rMan;
    drawData.main = &main;
    conType = worldInfo.conType;

    switch(conType) {
        case CONNECTIONTYPE_CLIENT: {
            clientStillConnecting = true;
            con.connect_p2p(netSource);
            set_name("");
            break;
        }
        case CONNECTIONTYPE_LOCAL: {
            if(!worldInfo.fileSource.empty())
                load_from_file(worldInfo.fileSource, worldInfo.fileDataBuffer);
            set_name(name);
            break;
        }
        case CONNECTIONTYPE_SERVER: {
            break;
        }
    }

    drawProg.init_client_callbacks();
    rMan.init_client_callbacks();
    bMan.init_client_callbacks();
    gridMan.init_client_callbacks();
    init_client_callbacks();
    con.client_send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_INITIAL_DATA, displayName, false);
}

void World::init_client_callbacks() {
    con.client_add_recv_callback(CLIENT_INITIAL_DATA, [&](cereal::PortableBinaryInputArchive& message) {
        bool isDirectConnect;
        message(isDirectConnect, ownID, userColor);
        if(!isDirectConnect) {
            std::string fileDisplayName;
            message(displayName, fileDisplayName, clients);
            set_name(fileDisplayName);
            Vector3f newBackColor;
            message(newBackColor);
            set_canvas_background_color(newBackColor, false);
            message(canvasScale);
            drawProg.initialize_draw_data(message);
            message(bMan, gridMan);
            if(!clients.empty()) {
                auto& [randomClientServerID, randomClient] = *clients.begin();
                drawData.cam.smooth_move_to(*main.world, randomClient.camCoords, randomClient.windowSize, true);
            }
        }
        nextClientID = get_max_id(ownID);
        clientStillConnecting = false;
    });
    con.client_add_recv_callback(CLIENT_USER_CONNECT, [&](cereal::PortableBinaryInputArchive& message) {
        ServerPortionID id;
        std::string displayName;
        Vector3f cursorColor;
        message(id, displayName, cursorColor);
        clients[id].displayName = displayName;
        clients[id].cursorColor = cursorColor;
        add_chat_message(clients[id].displayName, "joined", Toolbar::ChatMessage::Type::JOIN);
    });
    con.client_add_recv_callback(CLIENT_USER_DISCONNECT, [&](cereal::PortableBinaryInputArchive& message) {
        ServerPortionID id;
        message(id);
        auto it = clients.find(id);
        if(it != clients.end()) {
            add_chat_message(clients[id].displayName, "left", Toolbar::ChatMessage::Type::JOIN);
            clients.erase(id);
        }
    });
    con.client_add_recv_callback(CLIENT_MOVE_MOUSE, [&](cereal::PortableBinaryInputArchive& message) {
        ServerPortionID id;
        message(id);
        auto& c = clients[id];
        message(c.camCoords, c.windowSize, c.cursorPos);
    });
    con.client_add_recv_callback(CLIENT_CHAT_MESSAGE, [&](cereal::PortableBinaryInputArchive& message) {
        ServerPortionID senderID;
        std::string chatMessage;
        message(senderID, chatMessage);
        auto it = clients.find(senderID);
        if(it != clients.end())
            add_chat_message(clients[senderID].displayName, chatMessage, Toolbar::ChatMessage::Type::NORMAL);
    });
    con.client_add_recv_callback(CLIENT_CANVAS_COLOR, [&](cereal::PortableBinaryInputArchive& message) {
        Vector3f newBackColor;
        message(newBackColor);
        set_canvas_background_color(newBackColor, false);
    });
    con.client_add_recv_callback(CLIENT_CANVAS_SCALE, [&](cereal::PortableBinaryInputArchive& message) {
        uint64_t newCanvasScale;
        message(newCanvasScale);
        if(newCanvasScale > canvasScale) {
            con.client_send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_CANVAS_SCALE, newCanvasScale);
            scale_up(FixedPoint::pow_int(CANVAS_SCALE_UP_STEP, newCanvasScale - canvasScale));
            canvasScale = newCanvasScale;
        }
    });
    con.client_add_recv_callback(CLIENT_KEEP_ALIVE, [&](cereal::PortableBinaryInputArchive& message) {
    });
}

void World::focus_update() {
    con.update();

    if(con.is_host_disconnected()) {
        Logger::get().log("USERINFO", "Host connection failed");
        clientStillConnecting = false;
        clients.clear();
        netSource.clear();
        conType = CONNECTIONTYPE_LOCAL;
        con = ConnectionManager();
    }

    if(con.is_client_disconnected()) {
        Logger::get().log("USERINFO", "Client connection failed");
        clientStillConnecting = false;
        clients.clear();
        netSource.clear();
        conType = CONNECTIONTYPE_LOCAL;
        con = ConnectionManager();
    }

    if(!clientStillConnecting) {
        con.client_send_items_to_server(UNRELIABLE_COMMAND_CHANNEL, SERVER_MOVE_MOUSE, drawData.cam.c, main.window.size.cast<float>().eval(), main.input.mouse.pos);
        drawProg.update();
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

void World::undo_with_checks() {
    if(!clientStillConnecting && !drawProg.prevent_undo_or_redo())
        undo.undo();
}

void World::redo_with_checks() {
    if(!clientStillConnecting && !drawProg.prevent_undo_or_redo())
        undo.redo();
}

void World::unfocus_update() {
    con.update();
}

void World::set_canvas_background_color(const Vector3f& newBackColor, bool sendChangeOverNetwork) {
    if(newBackColor != convert_vec3<Vector3f>(canvasTheme.backColor)) {
        canvasTheme.backColor = SkColor4f{newBackColor.x(), newBackColor.y(), newBackColor.z(), 1.0f};
        Vector3f newHSV = rgb_to_hsv<Vector3f>(newBackColor);
        SkColor4f oldToolFrontColor = canvasTheme.toolFrontColor;
        if(newHSV.z() >= 0.6f)
            canvasTheme.toolFrontColor = SkColor4f{0.0f, 0.0f, 0.0f, 1.0f};
        else
            canvasTheme.toolFrontColor = SkColor4f{1.0f, 1.0f, 1.0f, 1.0f};
        if(oldToolFrontColor != canvasTheme.toolFrontColor)
            drawProg.clear_draw_cache();
        if(sendChangeOverNetwork)
            con.client_send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_CANVAS_COLOR, newBackColor);
    }
}

void World::send_chat_message(const std::string& message) {
    if(!clientStillConnecting) {
        con.client_send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_CHAT_MESSAGE, message);
        add_chat_message(displayName, message, Toolbar::ChatMessage::Type::NORMAL);
    }
}

void World::add_chat_message(const std::string& name, const std::string& message, Toolbar::ChatMessage::Type type) {
    Logger::get().log("CHAT", type == Toolbar::ChatMessage::JOIN ? (name + " " + message) : ("[" + name + "] " + message));
    chatMessages.emplace_front(Toolbar::ChatMessage{name, message, type});
    if(chatMessages.size() == CHAT_SIZE)
        chatMessages.pop_back();
}

ServerClientID World::get_new_id() {
    nextClientID++;
    return {ownID, nextClientID};
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

bool World::network_being_used() {
    return conType != CONNECTIONTYPE_LOCAL;
}

void World::set_name(const std::string& n) {
    if(n.empty())
        name = "New File";
    else
        name = n;
}

void World::start_hosting(const std::string& initNetSource, const std::string& serverLocalID) {
    if(conType != CONNECTIONTYPE_LOCAL)
        return;
    main.init_net_library();
    con.init_local_p2p(*this, serverLocalID);
    drawProg.init_client_callbacks();
    rMan.init_client_callbacks();
    bMan.init_client_callbacks();
    gridMan.init_client_callbacks();
    init_client_callbacks();
    con.client_send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_INITIAL_DATA, displayName, true);
    conType = CONNECTIONTYPE_SERVER;
    netSource = initNetSource;
    drawProg.components.set_server_from_client_list();
    clientStillConnecting = true;
}

void World::save_to_file(const std::filesystem::path& fileName) {
    try {
        filePath = force_extension_on_path(fileName, FILE_EXTENSION);

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

void World::load_from_file(const std::filesystem::path& fileName, std::string_view buffer) {
    filePath = force_extension_on_path(fileName, FILE_EXTENSION);

    std::string byteDataFromFile;

    if(buffer.empty()) {
        byteDataFromFile = read_file_to_string(fileName);
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
    nextClientID = get_max_id(ownID);

    Logger::get().log("USERINFO", "File loaded");
    set_name(filePath.stem().string());
}

ClientPortionID World::get_max_id(ServerPortionID serverID) {
    return std::max(std::max(rMan.get_max_id(serverID), drawProg.get_max_id(serverID)), gridMan.get_max_id(serverID));
}

void World::save_file(cereal::PortableBinaryOutputArchive& a) const {
    a(drawData.cam.c, main.window.size.cast<float>().eval());
    a(convert_vec3<Vector3f>(canvasTheme.backColor));
    drawProg.save_file(a);
    a(bMan);
    a(gridMan);
    rMan.save_strip_unused_resources(a, drawProg.get_used_resources());
}

void World::load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version) {
    CoordSpaceHelper coordsToJumpTo;
    Vector2f windowSizeToJumpTo;
    a(coordsToJumpTo, windowSizeToJumpTo);

    if(version < VersionNumber(0, 1, 0))
        set_canvas_background_color(DEFAULT_CANVAS_BACKGROUND_COLOR);
    else {
        Vector3f canvasBackColor;
        a(canvasBackColor);
        set_canvas_background_color(canvasBackColor);
    }

    drawData.cam.smooth_move_to(*this, coordsToJumpTo, windowSizeToJumpTo, true);
    drawProg.load_file(a, version);
    a(bMan);
    if(version >= VersionNumber(0, 2, 0))
        a(gridMan);
    a(rMan);
}

WorldScalar World::calculate_zoom_from_uniform_zoom(WorldScalar uniformZoom, WorldVec oldWindowSize) {
    WorldScalar a1(main.window.size.x() / static_cast<double>(oldWindowSize.x()));
    WorldScalar a2(main.window.size.y() / static_cast<double>(oldWindowSize.y()));
    return uniformZoom * ((a1 < a2) ? a1 : a2);
}

void World::draw_other_player_cursors(SkCanvas* canvas, const DrawData& drawData) {
    for(auto& [id, data] : clients) {
        if((drawData.cam.c.inverseScale << 10) > data.camCoords.inverseScale && drawData.clampDrawMinimum < data.camCoords.inverseScale) {
            canvas->save();
            data.camCoords.transform_sk_canvas(canvas, drawData);

            canvas->translate(data.cursorPos.x(), data.cursorPos.y());

            SkPaint p(SkColor4f{data.cursorColor.x(), data.cursorColor.y(), data.cursorColor.z(), 0.2f});
            canvas->drawCircle(0.0f, 0.0f, 4.5f, p);
            p.setStroke(true);
            p.setStrokeWidth(1.0f);
            p.setColor4f({data.cursorColor.x(), data.cursorColor.y(), data.cursorColor.z(), 0.7f});
            canvas->drawCircle(0.0f, 0.0f, 5.0f, p);

            SkFont f = main.toolbar.io->get_font(18.0f);
            SkFontMetrics metrics;
            f.getMetrics(&metrics);

            float nextText = f.measureText(data.displayName.c_str(), data.displayName.length(), SkTextEncoding::kUTF8, nullptr);
            Vector2f bounds{nextText, - metrics.fAscent + metrics.fDescent};

            SkPaint p2(SkColor4f{data.cursorColor.x(), data.cursorColor.y(), data.cursorColor.z(), 0.5f});
            canvas->drawSimpleText(data.displayName.c_str(), data.displayName.length(), SkTextEncoding::kUTF8, 6.0f, -metrics.fDescent, f, p2);

            SkPaint p3(color_mul_alpha(main.toolbar.io->theme->backColor1, 0.5f));
            canvas->drawRoundRect(SkRect::MakeXYWH(6.0f, -bounds.y(), bounds.x(), bounds.y()), 3.0f, 3.0f, p3);

            canvas->restore();
        }
    }
}

void World::scale_up_step() {
    canvasScale++;
    con.client_send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_CANVAS_SCALE, canvasScale);
    scale_up(WorldScalar(CANVAS_SCALE_UP_STEP));
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

    undo.clear(); // Do last to make sure that any undo generated while scaling up is removed, as it contains outdated scale info
}

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
