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
#include <functional>
#include <mutex>

class Logger {
    public:
        static Logger& get();
        enum class LogType {
            INFO = 0,
            DESKTOP_USERINFO,
            PHONE_USERINFO,
            USERINFO,
            WORLDFATAL,
            CHAT,
            FATAL
        };
        void set_log_function(LogType log, const std::function<void(const std::string&)> callback);
        void log(LogType log, const std::string& text);
        void cross_platform_println(const std::string& text);
    private:
        static Logger global;
        std::mutex logMutex;
        std::unordered_map<LogType, std::function<void(const std::string&)>> logs;
};
