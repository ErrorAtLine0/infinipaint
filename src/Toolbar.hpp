#pragma once
#include "GUIStuff/Elements/ScrollArea.hpp"
#include "DrawData.hpp"
#include "Helpers/FileDownloader.hpp"
#include <filesystem>
#include <modules/skparagraph/include/Paragraph.h>
#include <nlohmann/json.hpp>
#include <Helpers/Serializers.hpp>
#include <SDL3/SDL_dialog.h>
#include <Helpers/VersionNumber.hpp>

class MainProgram;

class Toolbar {
    public:
        struct ChatMessage {
            static constexpr float DISPLAY_TIME = 8.0f;
            static constexpr float FADE_START_TIME = 7.0f;
            std::string name;
            std::string message;
            enum Type {
                NORMAL = 0,
                JOIN 
            } type;
            TimePoint time;
        };

        struct ExtensionFilter {
            std::string name;
            std::string extensions;
        };

        std::string chatMessageInput;

        Toolbar(MainProgram& initMain);
        void update();
        void layout_run();
        struct ColorSelectorData {
            std::function<void()> onChange;
            std::function<void()> onSelect;
            std::function<void()> onDeselect;
        };
        struct ColorSelectorButtonData {
            std::function<void()> onSelectorButtonClick;
            std::function<void()> onChange;
            std::function<void()> onSelect;
            std::function<void()> onDeselect;
        };
        void color_selector_left(GUIStuff::Element* button, Vector4f* color, const ColorSelectorData& colorSelectorData = {});
        void color_selector_right(GUIStuff::Element* button, Vector4f* color, const ColorSelectorData& colorSelectorData = {});
        void color_button_left(const char* id, Vector4f* color, const ColorSelectorButtonData& colorSelectorData = {});
        void color_button_right(const char* id, Vector4f* color, const ColorSelectorButtonData& colorSelectorData = {});

        void paint_popup(Vector2f popupPos);

        typedef std::function<void(const std::filesystem::path&, const ExtensionFilter& extensionSelected)> OpenFileSelectorCallback;
        void open_file_selector(const std::string& filePickerName, const std::vector<ExtensionFilter>& extensionFilters, OpenFileSelectorCallback postSelectionFunc, const std::string& fileName = "", bool isSaving = false);
        void save_func();
        void save_as_func();

        void open_chatbox();
        void close_chatbox();

        GUIStuff::Element* colorLeftButton; 
        Vector4f* colorLeft = nullptr;
        ColorSelectorData colorLeftData;

        GUIStuff::Element* colorRightButton; 
        Vector4f* colorRight = nullptr;
        ColorSelectorData colorRightData;

        bool drawGui = true;

        bool app_close_requested();
    private:
        static void sdl_open_file_dialog_callback(void* userData, const char * const * fileList, int filter);

        void text_button_wide(const char* id, const char* str, const std::function<void()>& onClick);
        void reload_theme_list();
        void player_list();
        void chat_box();
        void global_log();
        void top_toolbar();
        void grid_menu(GUIStuff::Element* gridMenuButton);
        void add_grid();
        void stop_displaying_grid_menu();
        void stop_displaying_bookmark_menu();
        void stop_displaying_layer_menu();
        void bookmark_menu(GUIStuff::Element* bookmarkMenuButton);
        void layer_menu(GUIStuff::Element* layerMenuButton);
        void drawing_program_gui();
        void options_menu();
        void file_picker_gui_refresh_entries();
        void file_picker_gui_done();
        void file_picker_gui();
        void performance_metrics();
        void color_picker_window(const char* id, Vector4f** color, GUIStuff::Element* b, const ColorSelectorData& colorSelectorData);
        void color_palette(const char* id, Vector4f* color, const std::function<void()>& onChange);
        void open_world_file(bool isClient, const std::string& netSource, const std::string& serverLocalID);
        void load_default_theme();
        void about_menu_inner_gui();
        void web_version_welcome();
        void still_connecting_center_message();
        void no_layers_being_edited_message();
        void close_popup_gui();
        void add_world_to_close_popup_data(const std::shared_ptr<World>& w);
        void general_settings_inner_gui();
        void center_obstructing_window_gui(const char* id, Clay_SizingAxis x, Clay_SizingAxis y, const std::function<void()>& innerContent);

        struct ClosePopupData {
            struct CloseWorldData {
                std::weak_ptr<World> w;
                bool setToSave = true;
            };
            std::vector<CloseWorldData> worldsToClose; // Using a vector to ensure that the worlds are in proper order
            bool closeAppWhenDone = false;
        } closePopupData;

        #ifndef __EMSCRIPTEN__
        void update_notification_gui();
        void update_notification_check();
        struct UpdateCheckerData {
            bool showGui = false;
            bool updateCheckDone = false;
            std::string newVersionStr;
            std::shared_ptr<FileDownloader::DownloadData> versionFile;
        } updateCheckerData;
        #endif

        int selectedLicense = -1;

        std::string downloadNameSet;

        struct PaletteData {
            size_t selectedPalette = 0;
            bool addingPalette = false;
            std::string newPaletteStr;
        } paletteData;

        struct ThemeData {
            std::vector<std::string> themeDirList;
            std::optional<size_t> selectedThemeIndex;
            bool openedSaveAsMenu = false;
        } themeData;

        bool showPerformance = false;

        bool menuPopUpOpen = false;
        bool optionsMenuOpen = false;
        bool playerMenuOpen = false;

        float finalCalculatedGuiScale = 1.0f;

        bool chatboxOpen = false;

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
            GSETTINGS_TABLET,
            GSETTINGS_THEME,
            GSETTINGS_KEYBINDS,
            GSETTINGS_DEBUG
        } generalSettingsOptions = GSETTINGS_GENERAL;

        bool bookmarkMenuPopupOpen = false;
        bool layerMenuPopupOpen = false;

        struct GridMenu {
            bool popupOpen = false;
            std::string newName;
            uint32_t selectedGrid = std::numeric_limits<uint32_t>::max();
        } gridMenu;

        struct FilePicker {
            bool isOpen = false;
            std::string filePickerWindowName;
            std::vector<ExtensionFilter> extensionFiltersComplete;
            std::vector<std::string> extensionFilters;
            std::vector<std::filesystem::path> entries;
            std::filesystem::path currentSelectedPath;
            std::string fileName = "";
            bool isSaving = false;
            size_t extensionSelected;
            GUIStuff::ScrollArea* entriesScrollArea = nullptr;
            OpenFileSelectorCallback postSelectionFunc;
        } filePicker;

        struct NativeFilePicker {
            std::atomic<bool> isOpen = false;
            std::vector<ExtensionFilter> extensionFiltersComplete;
            std::vector<SDL_DialogFileFilter> sdlFileFilters;
            OpenFileSelectorCallback postSelectionFunc;
        };
        static NativeFilePicker nativeFilePicker;

        RichText::TextData build_paragraph_from_chat_message(const ChatMessage& message, float alpha);

        void load_icons_at(const std::filesystem::path& pathToLoad);

        std::string serverToConnectTo;
        std::string serverLocalID;

        MainProgram& main;
};
