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
#include <Helpers/NetworkingObjects/DelayUpdateSerializedClassManager.hpp>
#include <Helpers/NetworkingObjects/NetObjOrderedList.hpp>
#include "CoordSpaceHelper.hpp"
#include "CustomEvents.hpp"
#include "Helpers/NetworkingObjects/NetObjUnorderedSet.hpp"
#include "WorldUndoManager.hpp"
#include "Bookmarks/BookmarkManager.hpp"
#include "ResourceManager.hpp"
#include "DrawingProgram/DrawingProgram.hpp"
#include "Toolbar.hpp"
#include "SharedTypes.hpp"
#include "CanvasTheme.hpp"
#include "GridManager.hpp"
#include <Helpers/NetworkingObjects/NetObjManager.hpp>
#include <chrono>
#include <filesystem>
#include "ClientData.hpp"

class MainProgram;

//#define ENABLE_ORDERED_LIST_TEST

struct WorldScreenshotInfo;

class World {
    public:
        static constexpr std::string DOT_FILE_EXTENSION = ".infpnt";
        static constexpr std::string FILE_EXTENSION = "infpnt";
        static constexpr size_t CHAT_SIZE = 10;

        World(MainProgram& initMain, const CustomEvents::OpenInfiniPaintFileEvent& worldInfo);

        // NOTE: Keep at the very beginning so that it's destroyed last
        NetworkingObjects::NetObjManager netObjMan;

        void save_file(cereal::PortableBinaryOutputArchive& a) const;
        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version);

        MainProgram& main;
        DrawData drawData;
        WorldUndoManager undo;
        ResourceManager rMan;
        DrawingProgram drawProg;
        BookmarkManager bMan;
        GridManager gridMan;

        std::deque<Toolbar::ChatMessage> chatMessages;

        std::string netSource;

        void start_hosting(const std::string& initNetSource, const std::string& serverLocalID);

        void send_chat_message(const std::string& message);
        void add_chat_message(const std::string& name, const std::string& message, Toolbar::ChatMessage::Type type);

        WorldScalar calculate_zoom_from_uniform_zoom(WorldScalar uniformZoom, WorldVec oldWindowSize);

        void focus_update();
        void update();
        void on_tab_out();

        void draw(SkCanvas* canvas, const DrawData& calledDrawData);
        void early_destroy();

        bool clientStillConnecting = false;

        void autosave_to_directory(const std::filesystem::path& directoryToSaveAt);
        void save_to_file(const std::filesystem::path& filePathToSaveAt);
        void load_from_file(const std::filesystem::path& filePathToLoadFrom, std::string_view buffer);

        void undo_with_checks();
        void redo_with_checks();

        std::filesystem::path filePath;
        std::string name;

        NetworkingObjects::NetObjTemporaryPtr<ClientData> serverClientData; // For server, this is equal to ownClientData
        NetworkingObjects::NetObjTemporaryPtr<ClientData> ownClientData;
        NetworkingObjects::NetObjOwnerPtr<NetworkingObjects::NetObjUnorderedSet<ClientData>> clients;

        CanvasTheme canvasTheme;

        void scale_up(const WorldScalar& scaleUpAmount); // Should only be called from ClientData
        void scale_up_step(); // Call when scale up step happens

        bool should_ask_before_closing();
        void set_has_unsaved_local_changes(bool newHasUnsavedLocalChangesVal);
        bool is_focus();
        void set_to_layout_gui_if_focus();

        NetworkingObjects::DelayUpdateSerializedClassManager delayedUpdateObjectManager;

        std::shared_ptr<NetServer> netServer;
        std::shared_ptr<NetClient> netClient;

        void input_add_file_to_canvas_callback(const CustomEvents::AddFileToCanvasEvent& addFile);
        void input_paste_callback(const CustomEvents::PasteEvent& paste);
        void input_android_text_box_input_callback(const CustomEvents::AndroidTextBoxInputEvent& textboxInput);
        void input_drop_file_callback(const InputManager::DropCallbackArgs& drop);
        void input_drop_text_callback(const InputManager::DropCallbackArgs& drop);
        void input_text_key_callback(const InputManager::KeyCallbackArgs& key);
        void input_text_callback(const InputManager::TextCallbackArgs& text);
        void input_key_callback(const InputManager::KeyCallbackArgs& key);
        void input_mouse_button_callback(const InputManager::MouseButtonCallbackArgs& button);
        void input_mouse_motion_callback(const InputManager::MouseMotionCallbackArgs& motion);
        void input_mouse_wheel_callback(const InputManager::MouseWheelCallbackArgs& wheel);
        void input_pen_button_callback(const InputManager::PenButtonCallbackArgs& button);
        void input_pen_touch_callback(const InputManager::PenTouchCallbackArgs& touch);
        void input_pen_motion_callback(const InputManager::PenMotionCallbackArgs& motion);
        void input_pen_axis_callback(const InputManager::PenAxisCallbackArgs& axis);
        void input_multi_finger_touch_callback(const InputManager::MultiFingerTouchCallbackArgs& touch);
        void input_multi_finger_motion_callback(const InputManager::MultiFingerMotionCallbackArgs& motion);
        std::optional<InputManager::TextBoxStartInfo> get_text_box_start_info();
    private:
        bool saveThumbnail = false;
        bool hasUnsavedLocalChanges = false;

        void load_empty_canvas(const std::optional<std::filesystem::path>& filePathEmptyAutoSaveDir = std::nullopt);

        Vector3f get_random_cursor_color();
        void init_client_data_list();
        void init_client_data_list_callbacks();
        void init_net_obj_type_list();

        void init_client(const std::string& serverFullID);
        void set_name(const std::string& n);

        void draw_other_player_cursors(SkCanvas* canvas, const DrawData& drawData);
        void ensure_display_name_unique(std::string& displayName);

        bool connection_update();

        TimePoint timeToSendCameraData;

        std::chrono::steady_clock::time_point lastKeepAliveSent;

#ifdef ENABLE_ORDERED_LIST_TEST
        NetworkingObjects::NetObjOwnerPtr<NetworkingObjects::NetObjOrderedList<uint16_t>> listDebugTest;
        void list_debug_test_update();
        std::chrono::steady_clock::time_point listDebugTestTimeStart = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point nextSendTime = std::chrono::steady_clock::now();
#endif
};
