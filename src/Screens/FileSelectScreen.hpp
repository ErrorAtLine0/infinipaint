/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include "Screen.hpp"
#include "nlohmann/json.hpp"
#include "../GUIStuff/GUIFloatAnimation.hpp"
#include "../GUIStuff/Elements/ScrollArea.hpp"

class FileSelectScreen : public Screen {
    public:
        FileSelectScreen(MainProgram& m);
        virtual void gui_layout_run() override;
        virtual void update() override;
        virtual void draw(SkCanvas* canvas) override;
        virtual void input_paste_callback(const CustomEvents::PasteEvent& paste) override;
        virtual void input_open_infinipaint_file_callback(const CustomEvents::OpenInfiniPaintFileEvent& openFile) override;
        virtual void input_global_back_button_callback() override;
        virtual void input_app_about_to_go_to_background_callback() override;
        virtual void input_mobile_import_canvas_callback(const CustomEvents::MobileImportCanvasEvent& mobileImport) override;

        struct TrashInfo {
            struct TrashFile {
                SDL_Time trashTime;
                NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(TrashFile, trashTime)
            };
            std::unordered_map<std::string, TrashFile> files;
            NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(TrashInfo, files)
        };

        struct SaveInfo {
            TrashInfo trash;
            enum class FileSort {
                ALPHABETICAL_ASCENDING,
                ALPHABETICAL_DESCENDING,
                TIME_ASCENDING,
                TIME_DESCENDING
            } fileSort = FileSort::ALPHABETICAL_ASCENDING;
            enum class FileViewType {
                LARGE_GRID,
                MEDIUM_GRID,
                SMALL_GRID,
                LIST
            } fileViewType = FileViewType::MEDIUM_GRID;
            NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(SaveInfo, trash, fileSort, fileViewType)
        } saveInfo;

        ~FileSelectScreen();
    private:
        std::filesystem::path savePath;
        std::filesystem::path trashPath;
        std::filesystem::path infoPath;

        void save_files();

        bool connectOnPaste = false;
        std::string connectLobbyStr;

        struct FileInfo {
            std::string fileName;
            SDL_Time lastModifyTime;
            std::string lastModifyDate;
            bool selected = false;
        };

        void update_file_list(std::vector<FileInfo>& fL, const std::filesystem::path& folderPath, bool trashUpdate);
        void sort_file_list(std::vector<FileInfo>& fL);

        std::vector<FileInfo> fileList;

        GUIStuff::GUIFloatAnimation* mainMenuOpenAnim = nullptr;
        GUIStuff::GUIFloatAnimation* actionBarOpenAnim = nullptr;

        GUIStuff::ScrollArea* mainViewScrollArea = nullptr;

        void global_log();
        void main_display();
        void main_menu();
        void file_view();
        void connect_view();
        void settings_view();
        void about_view();
        void create_file_button();
        void file_view_edit();
        void menu_black_box();
        void edit_action_bar();
        void about_bottom_bar();
        void edit_title_bar();
        void title_bar();
        void text_transparent_option_button(const char* id, const char* text, const std::function<void()>& onClick);
        void icon_text_transparent_option_selected_button(const char* id, const std::string& svgPath, const char* text, bool isSelected, const std::function<void()>& onClick);
        void edit_action_bar_button(const char* id, const std::string& svgPath, const char* text, const std::function<void()>& onClick);

        enum class TrashMoveType {
            NONE,
            MOVE_TO_TRASH,
            MOVE_OUT_OF_TRASH
        };
        void move_selected_files(const std::filesystem::path& fromPath, const std::filesystem::path& toPath, TrashMoveType trashMoveType);
        void duplicate_selected_files(const std::filesystem::path& inPath);
        void share_selected_files();
        void delete_selected_files_in_trash();

        enum class MoreOptionsMenu {
            CLOSED,
            MAIN,
            VIEW,
            SORT
        } moreOptionsMenu = MoreOptionsMenu::CLOSED;

        enum class SelectedMenu {
            FILES,
            TRASH,
            CONNECT,
            SETTINGS,
            ABOUT
        } selectedMenu = SelectedMenu::FILES;

        int selectedLicense = -1;

        void start_edit_mode();
        size_t numberOfSelectedEntries;
        bool editMode = false;
};

NLOHMANN_JSON_SERIALIZE_ENUM(FileSelectScreen::SaveInfo::FileSort, {
    {FileSelectScreen::SaveInfo::FileSort::ALPHABETICAL_ASCENDING, "Alphabetical Ascending"},
    {FileSelectScreen::SaveInfo::FileSort::ALPHABETICAL_DESCENDING, "Alphabetical Descending"},
    {FileSelectScreen::SaveInfo::FileSort::TIME_ASCENDING, "Time Ascending"},
    {FileSelectScreen::SaveInfo::FileSort::TIME_DESCENDING, "Time Descending"},
})

NLOHMANN_JSON_SERIALIZE_ENUM(FileSelectScreen::SaveInfo::FileViewType, {
    {FileSelectScreen::SaveInfo::FileViewType::LARGE_GRID, "Large Grid"},
    {FileSelectScreen::SaveInfo::FileViewType::MEDIUM_GRID, "Medium Grid"},
    {FileSelectScreen::SaveInfo::FileViewType::SMALL_GRID, "Small Grid"},
    {FileSelectScreen::SaveInfo::FileViewType::LIST, "List"},
})
