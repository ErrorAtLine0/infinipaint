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
#include <SDL3/SDL_filesystem.h>
#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <SDL3/SDL_time.h>

std::vector<std::string> split_string_by_token(std::string str, std::string token);

// Hex character must be capitalized if it is a letter
uint8_t ascii_hex_char_to_number_no_checks(char asciiHex);
std::string byte_vector_to_hex_str(const std::vector<uint8_t>& byteVec);
std::string read_file_to_string(const std::filesystem::path& filePath);
bool is_valid_http_url(const std::string& str);
std::string remove_carriage_returns_from_str(std::string s);
std::string ensure_string_unique(const std::vector<std::string>& stringList, std::string str);
std::string sdl_time_to_nice_access_time(const SDL_DateTime& t, SDL_DateFormat dF, SDL_TimeFormat tF);
std::vector<std::string> glob_path_as_string_list(const std::filesystem::path& folder, const char* globStr, SDL_GlobFlags globFlags, const std::function<std::string(const std::filesystem::path)>& pathToStr);
std::filesystem::path sdl_safe_copy_file(const std::filesystem::path& toPath, const std::filesystem::path& fileToCopy, const std::string& fileName, const std::string& fileExtension);
