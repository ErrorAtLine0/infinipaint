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
#include "cereal/archives/portable_binary.hpp"
#include <include/core/SkColor.h>
#include <Helpers/ConvertVec.hpp>
#include <Helpers/NetworkingObjects/NetObjOwnerPtr.hpp>
#include <Helpers/Serializers.hpp>
#include <Helpers/VersionNumber.hpp>

#include <include/effects/SkRuntimeEffect.h>

class DrawingProgram;
class World;

class CanvasTheme {
    public:
        CanvasTheme(World& w);
        void server_init_no_file();
        SkColor4f get_back_color() const;
        const SkColor4f& get_tool_front_color() const;
        void set_back_color(const Vector3f& newBackColor);
        void register_class();
        void read_create_message(cereal::PortableBinaryInputArchive& a);
        void write_create_message(cereal::PortableBinaryOutputArchive& a) const;
        void save_file(cereal::PortableBinaryOutputArchive& a) const;
        void load_file(cereal::PortableBinaryInputArchive& a, VersionNumber version);

    private:
        void set_tool_front_color(DrawingProgram& drawP);
        struct BackColor {
            Vector3f c;
            template <typename Archive> void serialize(Archive& a) {
                a(c);
            }
        };
        NetworkingObjects::NetObjOwnerPtr<BackColor> backColor;
        SkColor4f toolFrontColor = {1.0f, 1.0f, 1.0f, 1.0f};
        World& world;
};
