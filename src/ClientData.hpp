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
#include <Helpers/Serializers.hpp>
#include "CoordSpaceHelper.hpp"
#include <Helpers/NetworkingObjects/NetObjTemporaryPtr.hpp>
#include <Helpers/NetworkingObjects/NetObjManager.hpp>

class ClientData {
    public:
        struct InitStruct {
            CoordSpaceHelper camCoords;
            Vector2f windowSize = {1000.0f, 1000.0f};
            Vector2f cursorPos = {1.0f, 1.0f};

            Vector3f cursorColor = {1.0f, 1.0f, 1.0f};
            std::string displayName;
            uint32_t gridSize = 0;
        };

        ClientData();
        ClientData(const InitStruct& initStruct);
        static void register_class(World& world);
        static void set_cursor_pos(const NetworkingObjects::NetObjTemporaryPtr<ClientData>& o, Vector2f newPos);
        static void set_window_size(const NetworkingObjects::NetObjTemporaryPtr<ClientData>& o, Vector2f newWindowSize);
        static void set_camera_coords(const NetworkingObjects::NetObjTemporaryPtr<ClientData>& o, const CoordSpaceHelper& newCoords);
        static void send_chat_message(const NetworkingObjects::NetObjTemporaryPtr<ClientData>& o, World& world, const std::string& chatMessage);
        static void scale_up_step(const NetworkingObjects::NetObjTemporaryPtr<ClientData>& o, World& world);
        void set_display_name(const std::string& newDisplayName);
        void set_cursor_color(const Vector3f& newCursorColor);

        const CoordSpaceHelper& get_cam_coords() const;
        const Vector2f& get_window_size() const;
        const Vector2f& get_cursor_pos() const;
        uint32_t get_grid_size() const;
        const std::string& get_display_name() const;
        const Vector3f& get_cursor_color() const;

        void draw_cursor(SkCanvas* canvas, const DrawData& drawData) const;

        template <typename Archive> void serialize(Archive& a) {
            a(camCoords, windowSize, cursorPos, cursorColor, displayName, gridSize);
        }
    private:
        void set_from_init_struct(const InitStruct& initStruct);
        CoordSpaceHelper camCoords;
        Vector2f windowSize;
        Vector2f cursorPos;

        Vector3f cursorColor;
        std::string displayName;
        uint32_t gridSize;
};

