#include "World.hpp"
#include "DrawComponents/DrawComponent.hpp"
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

#ifdef __EMSCRIPTEN__
    #include <EmscriptenHelpers/emscripten_browser_file.h>
#endif

World::World(MainProgram& initMain, OpenWorldInfo& worldInfo):
    main(initMain),
    rMan(*this),
    drawProg(*this),
    bMan(*this)
{
    displayName = main.displayName;

    netSource = worldInfo.netSource;

    drawData.cam.changed = [&]() {
        main.drawProgCache.refresh = true;
    };

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

    if(!clientStillConnecting && !drawProg.prevent_undo_or_redo()) {
        if(main.input.key(InputManager::KEY_UNDO).repeat)
            undo.undo();
        else if(main.input.key(InputManager::KEY_REDO).repeat)
            undo.redo();
    }
}

void World::unfocus_update() {
    con.update();
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

void World::download_file(const std::filesystem::path& fileName) {
    try {
        filePath = force_extension_on_path(fileName, FILE_EXTENSION);

        std::stringstream f;
        f.write(SAVEFILE_HEADER, SAVEFILE_HEADER_LEN);
        cereal::PortableBinaryOutputArchive a(f);
        SaveLoadFileOp op{.world = this};
        a(op);
        Logger::get().log("USERINFO", "File saved");

        set_name(filePath.stem().string());

        #ifdef __EMSCRIPTEN__
            emscripten_browser_file::download(
                filePath.string(),
                "application/octet-stream",
                f.view()
            );
        #endif
    }
    catch(const std::exception& e) {
        Logger::get().log("WORLDFATAL", std::string("Save error: ") + e.what());
    }
}

void World::save_to_file(const std::filesystem::path& fileName) {
    try {
        filePath = force_extension_on_path(fileName, FILE_EXTENSION);

        std::ofstream f(filePath, std::ios::out | std::ios::binary);
        f.write(SAVEFILE_HEADER, SAVEFILE_HEADER_LEN);
        cereal::PortableBinaryOutputArchive a(f);
        SaveLoadFileOp op{.world = this};
        a(op);
        f.close();
        Logger::get().log("USERINFO", "File saved");

        set_name(filePath.stem().string());
    }
    catch(const std::exception& e) {
        Logger::get().log("WORLDFATAL", std::string("Save error: ") + e.what());
    }
}

void World::load_from_file(const std::filesystem::path& fileName, std::string_view buffer) {
    filePath = force_extension_on_path(fileName, FILE_EXTENSION);


    if(buffer.empty()) {
        std::ifstream f(filePath, std::ios::in | std::ios::binary);
        std::string fileHeader;
        fileHeader.resize(SAVEFILE_HEADER_LEN);
        f.read(fileHeader.data(), SAVEFILE_HEADER_LEN);
        if(fileHeader != std::string(SAVEFILE_HEADER))
            throw std::runtime_error("[World::load_from_file] File does not have correct header");

        cereal::PortableBinaryInputArchive a(f);
        SaveLoadFileOp op{.world = this};
        a(op);
        f.close();
        nextClientID = std::max(rMan.get_max_id(ownID), drawProg.get_max_id(ownID));
    }
    else {
        ByteMemStream f((char*)buffer.data(), buffer.size());
        std::string fileHeader;
        fileHeader.resize(SAVEFILE_HEADER_LEN);
        f.read(fileHeader.data(), SAVEFILE_HEADER_LEN);
        if(fileHeader != std::string(SAVEFILE_HEADER))
            throw std::runtime_error("[World::load_from_file] File does not have correct header");

        cereal::PortableBinaryInputArchive a(f);
        SaveLoadFileOp op{.world = this};
        a(op);
        nextClientID = std::max(rMan.get_max_id(ownID), drawProg.get_max_id(ownID));
    }

    Logger::get().log("USERINFO", "File loaded");
    set_name(filePath.stem().string());
}

void World::SaveLoadFileOp::save(cereal::PortableBinaryOutputArchive& a) const {
    a(world->drawData.cam.c, world->main.window.size.cast<float>().eval());
    world->drawProg.write_to_file(a);
    a(world->bMan);
    world->rMan.save_strip_unused_resources(a, world->drawProg.get_used_resources());
}

void World::SaveLoadFileOp::load(cereal::PortableBinaryInputArchive& a) {
    CoordSpaceHelper coordsToJumpTo;
    Vector2f windowSizeToJumpTo;
    a(coordsToJumpTo, windowSizeToJumpTo);
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

void World::draw(SkCanvas* canvas) {
    drawData.refresh_draw_optimizing_values();

    drawProg.draw(canvas, drawData);

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
