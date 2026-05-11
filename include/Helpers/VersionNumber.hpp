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
#include <string>
#include <optional>

struct VersionNumber {
    unsigned major;
    unsigned minor;
    unsigned patch;
};

std::optional<VersionNumber> version_str_to_version_numbers(const std::string& versionStr);
std::string version_numbers_to_version_str(const VersionNumber& versionNum);
bool operator<=(const VersionNumber& a, const VersionNumber& b);
bool operator<(const VersionNumber& a, const VersionNumber& b);
bool operator>(const VersionNumber& a, const VersionNumber& b);
bool operator>=(const VersionNumber& a, const VersionNumber& b);
bool operator==(const VersionNumber& a, const VersionNumber& b);
bool operator!=(const VersionNumber& a, const VersionNumber& b);
