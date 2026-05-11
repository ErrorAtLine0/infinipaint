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

#include "Logger.hpp"
#include <stdexcept>
#include <iostream>
#ifdef __ANDROID__
#include <android/log.h>
#endif

Logger Logger::global;

Logger& Logger::get() {
    return global;
}

void Logger::add_log(const std::string& log, const std::function<void(const std::string&)> callback) {
    std::scoped_lock logLock(logMutex);
    logs[log] = callback;
}

void Logger::log(const std::string& log, const std::string& text) {
    std::scoped_lock logLock(logMutex);
    auto it = logs.find(log);
    if(it == logs.end())
        throw std::runtime_error("[Logger::log] Log " + log + " does not exist to log text: " + text);
    else
        it->second(text);
}

void Logger::cross_platform_println(const std::string& text) {
#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_INFO, "Logger", "%s", text.c_str());
#else
    std::cout << text << std::endl;
#endif
}
