#pragma once
#include "UndoManager.hpp"
#include "BookmarkManager.hpp"
#include "ResourceManager.hpp"
#include "ConnectionManager.hpp"
#include "DrawingProgram/DrawingProgram.hpp"
#include "Toolbar.hpp"
#include "SharedTypes.hpp"
#include <filesystem>

class MainProgram;

#define DEFAULT_CANVAS_BACKGROUND_COLOR Vector3f{0.12f, 0.12f, 0.12f}

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
            std::string fileSource;
            std::string netSource;
            std::string serverLocalID;
            std::string_view fileDataBuffer;
        };

        World(MainProgram& initMain, OpenWorldInfo& worldInfo);

        struct SaveLoadFileOp {
            World* world;
            std::string versionStr;
            void save(cereal::PortableBinaryOutputArchive& a) const;
            void load(cereal::PortableBinaryInputArchive& a);
        };

        MainProgram& main;
        DrawData drawData;
        UndoManager undo;
        ResourceManager rMan;
        DrawingProgram drawProg;
        BookmarkManager bMan;
        ConnectionManager con;

        std::deque<Toolbar::ChatMessage> chatMessages;

        std::string netSource;
        
        std::string displayName;

        void start_hosting(const std::string& initNetSource, const std::string& serverLocalID);

        void send_chat_message(const std::string& message);
        void add_chat_message(const std::string& message, Toolbar::ChatMessage::Color color);

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

        void save_to_file(const std::filesystem::path& fileName);
        void load_from_file(const std::filesystem::path& fileName, std::string_view buffer);

        void undo_with_checks();
        void redo_with_checks();

        ServerPortionID ownID;

        std::filesystem::path filePath;
        Vector3f userColor = {0.0f, 0.0f, 0.0f};
        std::string name;
        std::unordered_map<ServerPortionID, ClientData> clients;

        void set_canvas_background_color(const Vector3f& newBackColor, bool sendChangeOverNetwork = true);

        struct CanvasTheme {
            SkColor4f backColor;
            SkColor4f toolFrontColor;
        } canvasTheme;

    private:
        void init_client_callbacks();
        void set_name(const std::string& n);

        ClientPortionID nextClientID = 0;
        WorldVec mousePreviousWorldVec = {0, 0};
        WorldVec mouseWorldMove = {0, 0};
};
