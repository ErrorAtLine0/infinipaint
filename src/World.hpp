#pragma once
#include <Helpers/NetworkingObjects/DelayUpdateSerializedClassManager.hpp>
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include "Helpers/NetworkingObjects/NetObjUnorderedSet.hpp"
#include "UndoManager.hpp"
#include "BookmarkManager.hpp"
#include "ResourceManager.hpp"
#include "ConnectionManager.hpp"
#include "DrawingProgram/DrawingProgram.hpp"
#include "Toolbar.hpp"
#include "SharedTypes.hpp"
#include "CanvasTheme.hpp"
#include "GridManager.hpp"
#include <Helpers/NetworkingObjects/NetObjManager.hpp>
#include <filesystem>
#include "ClientData.hpp"

class MainProgram;

#define DEFAULT_CANVAS_BACKGROUND_COLOR Vector3f{0.07f, 0.07f, 0.07f}

class World {
    public:
        static constexpr std::string FILE_EXTENSION = "infpnt";
        static constexpr size_t CHAT_SIZE = 10;

        enum ConnectionType : int {
            CONNECTIONTYPE_CLIENT,
            CONNECTIONTYPE_SERVER,
            CONNECTIONTYPE_LOCAL
        };

        struct OpenWorldInfo {
            ConnectionType conType;
            std::optional<std::filesystem::path> filePathSource;
            std::string netSource;
            std::string serverLocalID;
            std::string_view fileDataBuffer;
        };

        World(MainProgram& initMain, OpenWorldInfo& worldInfo);

        // NOTE: Keep at the very beginning so that it's destroyed last
        NetworkingObjects::NetObjManager netObjMan;

        void save_file(cereal::PortableBinaryOutputArchive& a) const;
        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version);

        MainProgram& main;
        DrawData drawData;
        UndoManager undo;
        ResourceManager rMan;
        DrawingProgram drawProg;
        BookmarkManager bMan;
        ConnectionManager con;
        GridManager gridMan;

        std::deque<Toolbar::ChatMessage> chatMessages;

        std::string netSource;

        void start_hosting(const std::string& initNetSource, const std::string& serverLocalID);

        void send_chat_message(const std::string& message);
        void add_chat_message(const std::string& name, const std::string& message, Toolbar::ChatMessage::Type type);

        WorldVec get_mouse_world_pos();
        WorldVec get_mouse_world_move();

        WorldScalar calculate_zoom_from_uniform_zoom(WorldScalar uniformZoom, WorldVec oldWindowSize);

        void focus_update();
        void unfocus_update();

        void draw(SkCanvas* canvas);
        void early_destroy();

        bool network_being_used();

        ConnectionType conType;
        bool clientStillConnecting = false;

        void save_to_file(const std::filesystem::path& filePathToSaveAt);
        void load_from_file(const std::filesystem::path& filePathToLoadFrom, std::string_view buffer);

        void undo_with_checks();
        void redo_with_checks();

        std::filesystem::path filePath;
        std::string name;

        NetworkingObjects::NetObjTemporaryPtr<ClientData> ownClientData;
        NetworkingObjects::NetObjOwnerPtr<NetworkingObjects::NetObjUnorderedSet<ClientData>> clients;

        CanvasTheme canvasTheme;

        void scale_up(const WorldScalar& scaleUpAmount);
        void scale_up_step();

        bool setToDestroy = false;
        NetworkingObjects::DelayUpdateSerializedClassManager delayedUpdateObjectManager;

        std::shared_ptr<NetServer> netServer;
    private:
        Vector3f get_random_cursor_color();
        void init_client_data_list();
        void init_client_data_list_callbacks();
        void init_net_obj_type_list();

        void init_client_callbacks();
        void init_server_callbacks();
        void set_name(const std::string& n);

        void draw_other_player_cursors(SkCanvas* canvas, const DrawData& drawData);
        void ensure_display_name_unique(std::string& displayName);

        TimePoint timeToSendCameraData;
        WorldVec mousePreviousWorldVec = {0, 0};
        WorldVec mouseWorldMove = {0, 0};

        std::chrono::steady_clock::time_point lastKeepAliveSent;
};
