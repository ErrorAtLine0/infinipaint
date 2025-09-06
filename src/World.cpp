#include "World.hpp"
#include "DrawComponents/DrawComponent.hpp"
#include "Helpers/HsvRgb.hpp"
#include "Helpers/MathExtras.hpp"
#include "Helpers/Networking/ByteStream.hpp"
#include "Server/CommandList.hpp"
#include "MainProgram.hpp"
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
        NetLibrary::init();

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
            drawProg.initialize_draw_data(message);
            message(bMan);
        }
        nextClientID = std::max(rMan.get_max_id(ownID), drawProg.get_max_id(ownID));
        clientStillConnecting = false;
    });
    con.client_add_recv_callback(CLIENT_USER_CONNECT, [&](cereal::PortableBinaryInputArchive& message) {
        ServerPortionID id;
        std::string displayName;
        Vector3f cursorColor;
        message(id, displayName, cursorColor);
        clients[id].displayName = displayName;
        clients[id].cursorColor = cursorColor;
        add_chat_message(clients[id].displayName + " joined", Toolbar::ChatMessage::COLOR_JOIN);
    });
    con.client_add_recv_callback(CLIENT_USER_DISCONNECT, [&](cereal::PortableBinaryInputArchive& message) {
        ServerPortionID id;
        message(id);
        auto it = clients.find(id);
        if(it != clients.end()) {
            add_chat_message(clients[id].displayName + " left", Toolbar::ChatMessage::COLOR_JOIN);
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
        std::string chatMessage;
        message(chatMessage);
        add_chat_message(chatMessage, Toolbar::ChatMessage::COLOR_NORMAL);
    });
    con.client_add_recv_callback(CLIENT_CANVAS_COLOR, [&](cereal::PortableBinaryInputArchive& message) {
        Vector3f newBackColor;
        message(newBackColor);
        set_canvas_background_color(newBackColor, false);
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
        std::string newMessage = "[" + displayName + "] " + message;
        con.client_send_items_to_server(RELIABLE_COMMAND_CHANNEL, SERVER_CHAT_MESSAGE, newMessage);
        add_chat_message(newMessage, Toolbar::ChatMessage::COLOR_NORMAL);
    }
}

void World::add_chat_message(const std::string& message, Toolbar::ChatMessage::Color color) {
    Logger::get().log("CHAT", message);
    chatMessages.emplace_front(Toolbar::ChatMessage{message, color});
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
    NetLibrary::init();
    con.init_local_p2p(*this, serverLocalID);
    drawProg.init_client_callbacks();
    rMan.init_client_callbacks();
    bMan.init_client_callbacks();
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
        f.write(SAVEFILE_HEADER, SAVEFILE_HEADER_LEN);

        {
            std::stringstream fWorldDataToCompress;
            {
                cereal::PortableBinaryOutputArchive a(fWorldDataToCompress);
                SaveLoadFileOp op{.world = this, .versionStr = SAVEFILE_HEADER};
                a(op);
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

    if(buffer.size() < SAVEFILE_HEADER_LEN)
        throw std::runtime_error("[World::load_from_file] File too small");

    std::string_view fileHeader = buffer.substr(0, SAVEFILE_HEADER_LEN);
    std::string_view uncompressedDataView;
    std::vector<char> uncompressedDataVector;

    if(fileHeader == SAVEFILE_HEADER) {
        uncompressedDataVector.resize(ZSTD_getFrameContentSize(buffer.data() + SAVEFILE_HEADER_LEN, buffer.size() - SAVEFILE_HEADER_LEN));
        size_t trueUncompressedSize = ZSTD_decompress(uncompressedDataVector.data(), uncompressedDataVector.size(), buffer.data() + SAVEFILE_HEADER_LEN, buffer.size() - SAVEFILE_HEADER_LEN);
        uncompressedDataView = std::string_view(uncompressedDataVector.data(), trueUncompressedSize);
    }
    else if(fileHeader == SAVEFILE_HEADER_V1)
        uncompressedDataView = std::string_view(buffer.data() + SAVEFILE_HEADER_LEN, buffer.size() - SAVEFILE_HEADER_LEN);
    else
        throw std::runtime_error("[World::load_from_file] File does not have correct header");



    ByteMemStream f((char*)uncompressedDataView.data(), uncompressedDataView.size());

    cereal::PortableBinaryInputArchive a(f);
    SaveLoadFileOp op{.world = this, .versionStr = std::string(fileHeader)};
    a(op);
    nextClientID = std::max(rMan.get_max_id(ownID), drawProg.get_max_id(ownID));

    Logger::get().log("USERINFO", "File loaded");
    set_name(filePath.stem().string());
}

void World::SaveLoadFileOp::save(cereal::PortableBinaryOutputArchive& a) const {
    a(world->drawData.cam.c, world->main.window.size.cast<float>().eval());
    a(convert_vec3<Vector3f>(world->canvasTheme.backColor));
    world->drawProg.write_to_file(a);
    a(world->bMan);
    world->rMan.save_strip_unused_resources(a, world->drawProg.get_used_resources());
}

void World::SaveLoadFileOp::load(cereal::PortableBinaryInputArchive& a) {
    CoordSpaceHelper coordsToJumpTo;
    Vector2f windowSizeToJumpTo;
    a(coordsToJumpTo, windowSizeToJumpTo);
    if(versionStr == SAVEFILE_HEADER) {
        Vector3f canvasBackColor;
        a(canvasBackColor);
        world->set_canvas_background_color(canvasBackColor);
    }
    else
        world->set_canvas_background_color(DEFAULT_CANVAS_BACKGROUND_COLOR);
    world->drawData.cam.smooth_move_to(*world, coordsToJumpTo, windowSizeToJumpTo, true);
    world->drawProg.initialize_draw_data(a);
    a(world->bMan);
    a(world->rMan);
}

WorldScalar World::calculate_zoom_from_uniform_zoom(WorldScalar uniformZoom, WorldVec oldWindowSize) {
    WorldScalar a1(main.window.size.x() / static_cast<double>(oldWindowSize.x()));
    WorldScalar a2(main.window.size.y() / static_cast<double>(oldWindowSize.y()));
    return uniformZoom * ((a1 < a2) ? a1 : a2);
}

void World::draw_other_player_cursors(SkCanvas* canvas, const DrawData& drawData) {
    for(auto& [id, data] : clients) {
        if(drawData.clampDrawMaximum > data.camCoords.inverseScale && drawData.clampDrawMinimum < data.camCoords.inverseScale) {
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

void World::draw(SkCanvas* canvas) {
    drawData.refresh_draw_optimizing_values();
    if(drawData.drawGrids)
        gridMan.draw(canvas, drawData);
    drawProg.draw(canvas, drawData);
    draw_other_player_cursors(canvas, drawData);
}
