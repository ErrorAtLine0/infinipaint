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

#include "StringHelpers.hpp"
#include <SDL3/SDL_time.h>
#include <SDL3/SDL_timer.h>
#include <chrono>
#include <SDL3/SDL_filesystem.h>
#include <regex>
#include <SDL3/SDL_iostream.h>

std::vector<std::string> split_string_by_token(std::string str, std::string token) {
    // slightly modified from: https://stackoverflow.com/a/46943631
    std::vector<std::string> toRet;
    while(!str.empty()) {
        size_t index = str.find(token);
        if(index != std::string::npos) {
            toRet.emplace_back(str.substr(0, index));
            str = str.substr(index + token.size());
            if(str.empty())
                toRet.emplace_back(str);
        }
        else {
            toRet.emplace_back(str);
            return toRet;
        }
    }
    return toRet;
}

uint8_t ascii_hex_char_to_number_no_checks(char asciiHex) {
    if(asciiHex <= '9')
        return static_cast<uint8_t>(asciiHex - '0');
    else
        return static_cast<uint8_t>(asciiHex - 'A');
}

std::string byte_vector_to_hex_str(const std::vector<uint8_t>& byteVec) {
    static constexpr char charList[] = "0123456789ABCDEF";

    std::string ret;
    ret.reserve(byteVec.size() * 2);

    for(uint8_t b : byteVec) {
        ret.push_back(charList[b >> 4]);
        ret.push_back(charList[b & 0x0F]);
    }

    return ret;
}

std::string read_file_to_string(const std::filesystem::path& filePath) {
    // Could also use SDL_LoadFile, but that allocates into a void* and involves a copy into std::string

    std::string toRet;
    SDL_IOStream* file = SDL_IOFromFile(filePath.string().c_str(), "rb");
    if(!file)
        throw std::runtime_error("[read_file_to_string] Could not open file " + filePath.string());
    Sint64 fileSize = SDL_GetIOSize(file);
    if(fileSize < 0)
        throw std::runtime_error("[read_file_to_string] tellg failed for file " + filePath.string());
    toRet.resize(fileSize);
    SDL_ReadIO(file, toRet.data(), fileSize);
    SDL_CloseIO(file);
    return toRet;
}

bool is_valid_http_url(const std::string& str) {
    if(str.empty())
        return false;

    // Regex to check valid URL (from https://stackoverflow.com/a/3809435)
    std::regex pattern(R"rawstr(https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b([-a-zA-Z0-9()@:%_\+.~#?&//=]*))rawstr");

    return std::regex_match(str, pattern);
}

std::string remove_carriage_returns_from_str(std::string s) {
    std::erase(s, '\r');
    return s;
}

std::string ensure_string_unique(const std::vector<std::string>& stringList, std::string str) {
    for(;;) {
        bool isUnique = true;
        for(const std::string& strToCheckAgainst : stringList) {
            if(strToCheckAgainst == str) {
                size_t leftParenthesisIndex = str.find_last_of('(');
                size_t rightParenthesisIndex = str.find_last_of(')');
                bool failToIncrement = true;
                if(leftParenthesisIndex != std::string::npos && rightParenthesisIndex != std::string::npos && leftParenthesisIndex < rightParenthesisIndex) {
                    std::string numStr = str.substr(leftParenthesisIndex + 1, rightParenthesisIndex - leftParenthesisIndex - 1);
                    bool isAllDigits = true;
                    for(char c : numStr) {
                        if(!isdigit(c)) {
                            isAllDigits = false;
                            break;
                        }
                    }
                    if(isAllDigits) {
                        try {
                            int s = std::stoi(numStr);
                            s++;
                            str = str.substr(0, leftParenthesisIndex);
                            str += "(" + std::to_string(s) + ")";
                            failToIncrement = false;
                        }
                        catch(...) {}
                    }
                }
                if(failToIncrement)
                    str += " (2)";
                isUnique = false;
                break;
            }
        }
        if(isUnique)
            break;
    }
    return str;
}

std::string sdl_time_to_nice_access_time(const SDL_DateTime& t, SDL_DateFormat dF, SDL_TimeFormat tF) {
    SDL_DateTime currentDt;
    {
        SDL_Time currentTime;
        SDL_GetCurrentTime(&currentTime);
        SDL_TimeToDateTime(currentTime, &currentDt, true);
    }
    if(currentDt.year == t.year && currentDt.month == t.month && currentDt.day == t.day) {
        switch(tF) {
            case SDL_TIME_FORMAT_24HR:
                return std::format("{0}:{1:0>2}", t.hour, t.minute);
            case SDL_TIME_FORMAT_12HR: {
                std::string pmAm = t.hour >= 12 ? "PM" : "AM";
                int displayedHour;
                if(t.hour == 0)
                    displayedHour = 12;
                else if(t.hour > 12)
                    displayedHour = t.hour - 12;
                else
                    displayedHour = t.hour;
                return std::format("{0}:{1:0>2} {2}", displayedHour, t.minute, pmAm);
            }
        }
    }
    else {
        switch(dF) {
            case SDL_DATE_FORMAT_YYYYMMDD:
                return std::format("{0}-{1}-{2}", t.year, t.month, t.day);
            case SDL_DATE_FORMAT_DDMMYYYY:
                if(currentDt.year == t.year)
                    return std::format("{0} {1}", t.day, std::chrono::month(t.month));
                else
                    return std::format("{0} {1}, {2}", t.day, std::chrono::month(t.month), t.year);
            case SDL_DATE_FORMAT_MMDDYYYY: {
                if(currentDt.year == t.year)
                    return std::format("{1} {0}", t.day, std::chrono::month(t.month));
                else
                    return std::format("{1} {0}, {2}", t.day, std::chrono::month(t.month), t.year);
            }
        }
    }
    return "";
}

std::vector<std::string> glob_path_as_string_list(const std::filesystem::path& folder, const char* globStr, SDL_GlobFlags globFlags, const std::function<std::string(const std::filesystem::path)>& pathToStr) {
    int globCount;
    char** filesInPath = SDL_GlobDirectory(folder.string().c_str(), globStr, globFlags, &globCount);
    if(!filesInPath)
        throw std::runtime_error("[glob_path_as_string_list] SDL_GlobDirectory failed");
    std::vector<std::string> toRet(globCount);
    for(int i = 0; i < globCount; i++)
        toRet[i] = pathToStr(std::filesystem::path(filesInPath[i]));
    return toRet;
}

std::filesystem::path sdl_safe_copy_file(const std::filesystem::path& toPath, const std::filesystem::path& fileToCopy, const std::string& fileName, const std::string& fileExtension) {
    std::vector<std::string> toFolderListNames;
    try {
        toFolderListNames = glob_path_as_string_list(toPath, "*", 0, [&](const auto& p){ return p.stem().string();});
    } catch(...) {
        SDL_CreateDirectory(toPath.string().c_str());
    }
    std::string newFileName = ensure_string_unique(toFolderListNames, std::filesystem::path(fileName).stem().string());
    std::filesystem::path newPath = toPath / (newFileName + "." + fileExtension);
    SDL_CopyFile(fileToCopy.string().c_str(), newPath.string().c_str());
    return newPath;
}
