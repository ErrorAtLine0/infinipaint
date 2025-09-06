#pragma once
#include "GUIStuff/GUIManager.hpp"
#include "DrawData.hpp"
#include "Server/CommandList.hpp"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <Helpers/Serializers.hpp>
#include <SDL3/SDL_dialog.h>

class MainProgram;

class Toolbar {
    public:
        struct LogMessage {
            std::string text;
            enum {
                COLOR_NORMAL = 0,
                COLOR_ERROR
            } color;
            TimePoint time;
        };

        struct ChatMessage {
            std::string text;
            enum Color {
                COLOR_NORMAL = 0,
                COLOR_JOIN
            } color;
            TimePoint time;
        };

        struct ExtensionFilter {
            std::string name;
            std::string extensions;
        };

        std::string chatMessageInput;

        Toolbar(MainProgram& initMain);
        void update();
        void draw(SkCanvas* canvas);
        void color_selector_left(Vector4f* color);
        void color_selector_right(Vector4f* color);

        typedef std::function<void(const std::filesystem::path&, const ExtensionFilter& extensionSelected)> OpenFileSelectorCallback;
        void open_file_selector(const std::string& filePickerName, const std::vector<ExtensionFilter>& extensionFilters, OpenFileSelectorCallback postSelectionFunc, const std::string& fileName = "", bool isSaving = false);
        void save_func();
        void save_as_func();
        std::filesystem::path& file_selector_path();
        std::shared_ptr<GUIStuff::UpdateInputData> io;
        GUIStuff::GUIManager gui;

        nlohmann::json get_config_json();
        void set_config_json(const nlohmann::json& j);

        void save_palettes();
        void load_palettes();
        void save_theme();
        bool load_theme();
        void load_licenses();

        bool velocityAffectsBrushWidth = false;
        double dragZoomSpeed = 0.02;
        double scrollZoomSpeed = 0.4;
        float jumpTransitionTime = 0.5f;
        Vector4f jumpTransitionEasing{0.75, 0.25, 0.25, 0.75};
        bool viewWebVersionWelcome = false;

        struct TabletOptions {
            bool pressureAffectsBrushWidth = true;
            float smoothingSamplingTime = 0.04f;
            uint8_t middleClickButton = 1;
            uint8_t rightClickButton = 2;
            bool ignoreMouseMovementWhenPenInProximity = false;
        } tabletOptions;

        Vector4f* colorLeft = nullptr;
        Vector4f* colorRight = nullptr;
        std::optional<Vector2f> paintPopupLocation;

        float final_gui_scale();
    private:
        static void sdl_open_file_dialog_callback(void* userData, const char * const * fileList, int filter);

        void reload_theme_list();
        void player_list();
        void chat_box();
        void global_log();
        void top_toolbar();
        void grid_menu(bool justOpened);
        void bookmark_menu(bool justOpened);
        void drawing_program_gui();
        void options_menu();
        void file_picker_gui();
        void performance_metrics();
        void color_palette(const std::string& id, Vector4f* color, bool& hoveringOnDropdown);
        void open_world_file(int conType, const std::string& netSource, const std::string& serverLocalID);
        void load_default_palette();
        void load_default_theme();
        void paint_popup();
        void about_menu_gui();
        void web_version_welcome();

        std::string ownLicenseText;
        std::vector<std::pair<std::string, std::string>> thirdPartyLicenses;
        int selectedLicense = -1;

        std::string downloadNameSet;

        struct PaletteData {
            struct Palette {
                std::string name;
                std::vector<Vector3f> colors;
                NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Palette, name, colors)
            };

            size_t selectedPalette = 0;
            int selectedColor = -1;
            std::vector<Palette> palettes;
            bool addingPalette = false;
            std::string newPaletteStr;
        } paletteData;

        struct ThemeData {
            std::vector<std::string> themeDirList;
            std::optional<size_t> selectedThemeIndex;
            std::string themeCurrentlyLoaded = "Default";
            bool openedSaveAsMenu = false;
        } themeData;

        bool justAssignedColorLeft = false;
        bool justAssignedColorRight = false;

        bool showPerformance = false;

        bool menuPopUpOpen = false;
        bool optionsMenuOpen = false;
        bool playerMenuOpen = false;

        float guiScale = 1.0f;

        enum {
            CHATBOXSTATE_OPEN,
            CHATBOXSTATE_JUSTOPEN,
            CHATBOXSTATE_CLOSE
        } chatBoxState = CHATBOXSTATE_CLOSE;

        enum OptionsMenuType {
            HOST_MENU,
            CONNECT_MENU,
            GENERAL_SETTINGS_MENU,
            LOBBY_INFO_MENU,
            CANVAS_SETTINGS_MENU,
            SET_DOWNLOAD_NAME,
            ABOUT_MENU
        } optionsMenuType;
        enum GeneralSettingsOptions {
            GSETTINGS_GENERAL = 0,
            GSETTINGS_APPEARANCE,
            GSETTINGS_TABLET,
            GSETTINGS_THEME,
            GSETTINGS_KEYBINDS,
            GSETTINGS_DEBUG
        } generalSettingsOptions = GSETTINGS_GENERAL;

        struct BookmarkMenu {
            bool popupOpen = false;
            std::string currentSelectedBookmark;
            std::string newName;
        } bookMenu;

        struct GridMenu {
            bool popupOpen = false;
            std::string currentSelectedGrid;
            std::string newName;
        } gridMenu;

        bool useNativeFilePicker = true;
        struct FilePicker {
            bool isOpen = false;
            std::string filePickerWindowName;
            std::vector<ExtensionFilter> extensionFiltersComplete;
            std::vector<std::string> extensionFilters;
            std::vector<std::filesystem::path> entries;
            bool refreshEntries = true;
            std::filesystem::path currentSearchPath;
            std::filesystem::path currentSelectedPath;
            std::string fileName;
            size_t extensionSelected;
            OpenFileSelectorCallback postSelectionFunc;
        } filePicker;

        struct NativeFilePicker {
            std::atomic<bool> isOpen = false;
            std::vector<ExtensionFilter> extensionFiltersComplete;
            std::vector<SDL_DialogFileFilter> sdlFileFilters;
            OpenFileSelectorCallback postSelectionFunc;
        };
        static NativeFilePicker nativeFilePicker;

        std::optional<unsigned> keybindWaiting;

        std::filesystem::path testing;

        void start_gui();
        void end_gui();

        std::string serverToConnectTo;
        std::string serverLocalID;

        MainProgram& main;
};
