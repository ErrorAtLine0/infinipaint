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

void text_label_color(GUIManager& gui, std::string_view val, const SkColor4f& color);
void text_label_size(GUIManager& gui, std::string_view val, float modifier);
void text_label_light(GUIManager& gui, std::string_view val);
void text_label(GUIManager& gui, std::string_view val);
void text_label_light_centered(GUIManager& gui, std::string_view val);
void text_label_centered(GUIManager& gui, std::string_view val);

void mutable_text_label(GUIManager& gui, const char* id, const std::string& val);
void mutable_text_label_light(GUIManager& gui, const char* id, const std::string& val);

void ellipse_wide_paragraph_label(GUIManager& gui, const char* id, const std::string& val);

}}
