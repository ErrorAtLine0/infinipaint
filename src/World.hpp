#pragma once
#include "Helpers/NetworkingObjects/NetObjOrderedList.hpp"
#include "UndoManager.hpp"
#include "BookmarkManager.hpp"
#include "ResourceManager.hpp"
#include "ConnectionManager.hpp"
#include "DrawingProgram/DrawingProgram.hpp"
#include "Toolbar.hpp"
#include "SharedTypes.hpp"
#include "GridManager.hpp"
#include <Helpers/NetworkingObjects/NetObjManager.hpp>
#include <filesystem>

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
        NetworkingObjects::NetObjManager netObjMan;

        std::deque<Toolbar::ChatMessage> chatMessages;

        std::string netSource;
        
        std::string displayName;

        void start_hosting(const std::string& initNetSource, const std::string& serverLocalID);

        void send_chat_message(const std::string& message);
        void add_chat_message(const std::string& name, const std::string& message, Toolbar::ChatMessage::Type type);

        WorldVec get_mouse_world_pos();
        WorldVec get_mouse_world_move();

        WorldScalar calculate_zoom_from_uniform_zoom(WorldScalar uniformZoom, WorldVec oldWindowSize);

        ServerClientID get_new_id();

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

        ServerPortionID ownID = 0;

        std::filesystem::path filePath;
        Vector3f userColor = {0.0f, 0.0f, 0.0f};
        std::string name;
        std::unordered_map<ServerPortionID, ClientData> clients;

        NetworkingObjects::NetObjPtr<NetworkingObjects::NetObjOrderedList<std::string>> stringListTest;

        void set_canvas_background_color(const Vector3f& newBackColor, bool sendChangeOverNetwork = true);

        struct CanvasTheme {
            SkColor4f backColor = {0.0f, 0.0f, 0.0f, 1.0f};
            SkColor4f toolFrontColor = {1.0f, 1.0f, 1.0f, 1.0f};
        } canvasTheme;

        void scale_up(const WorldScalar& scaleUpAmount);
        void scale_up_step();

        uint64_t canvasScale = 0;
    private:

        ClientPortionID get_max_id(ServerPortionID serverID);

        void init_client_callbacks();
        void set_name(const std::string& n);

        void draw_other_player_cursors(SkCanvas* canvas, const DrawData& drawData);

        TimePoint timeToSendCameraData;
        struct SentCameraData {
            CoordSpaceHelper camTransform;
            Vector2f windowSize;
        };
        std::optional<SentCameraData> sentCameraData;

        ClientPortionID nextClientID = 0;
        WorldVec mousePreviousWorldVec = {0, 0};
        WorldVec mouseWorldMove = {0, 0};
};
