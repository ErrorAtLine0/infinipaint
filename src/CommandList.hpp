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
#include <cstdint>
#include <Helpers/Networking/NetLibrary.hpp>

enum ServerCommands : MessageCommandType {
    SERVER_FRAGMENT = 0,
    SERVER_KEEP_ALIVE,
    SERVER_INITIAL_DATA,
    SERVER_UPDATE_NETWORK_OBJECT,
    SERVER_UPDATE_MANY_NETWORK_OBJECTS,
    SERVER_NEW_RESOURCE_ID,
    SERVER_NEW_RESOURCE_DATA,
    SERVER_TRANSFORM_MANY_COMPONENTS
};

enum ClientCommands : MessageCommandType {
    CLIENT_FRAGMENT = 0,
    CLIENT_KEEP_ALIVE,
    CLIENT_INITIAL_DATA,
    CLIENT_UPDATE_NETWORK_OBJECT,
    CLIENT_UPDATE_MANY_NETWORK_OBJECTS,
    CLIENT_NEW_RESOURCE_ID,
    CLIENT_NEW_RESOURCE_DATA,
    CLIENT_TRANSFORM_MANY_COMPONENTS
};
