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
#include "../GUIManager.hpp"

namespace GUIStuff { namespace ElementHelpers {

void checkbox_boolean(GUIManager& gui, const char* id, bool* d, const std::function<void()>& onClick = nullptr);
void checkbox_field(GUIManager& gui, const char* id, std::string_view name, const std::function<bool()>& isTicked, const std::function<void()>& onClick);
void checkbox_boolean_field(GUIManager& gui, const char* id, std::string_view name, bool* d, const std::function<void()>& onClick = nullptr);

}}
