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

#include "NetThreadManager.hpp"
#include <Helpers/Networking/NetLibrary.hpp>
#include <Helpers/Logger.hpp>
#include "AndroidJNICalls.hpp"
#include "MainProgram.hpp"

NetThreadManager NetThreadManager::global;

NetThreadManager& NetThreadManager::get() {
    return global;
}

void NetThreadManager::init(MainProgram* m) {
    if(started)
        return;
    started = true;
    main = m;
#ifdef __ANDROID__
    AndroidJNICalls::startNetworkService();
#else
    NetLibrary::init_websocket();
#endif
}

void NetThreadManager::synchronous_update() {
#ifndef __ANDROID__
    NetLibrary::update();
#endif
}

void NetThreadManager::init_thread() {
    if(t)
        throw std::runtime_error("[NetThreadManager::init_thread] Thread can't exist when init_thread is called");
    Logger::get().log("INFO", "[NetThreadManager::init_thread] Starting network thread");
    destroyThread = false;
    t = std::make_unique<std::thread>([&]{ thread_update(); });
}

void NetThreadManager::thread_update() {
    NetLibrary::init_websocket();
    for(;;) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(20ms);
        if(destroyThread) {
            NetLibrary::destroy_websocket();
            break;
        }
        {
            std::scoped_lock l{backgroundUpdateMutex};
            if(backgroundUpdate)
                main->background_update();
        }
        NetLibrary::update();
    }
#ifdef __ANDROID__
    AndroidJNICalls::stopNetworkService();
#endif
}

void NetThreadManager::destroy_thread() {
    if(!t)
        throw std::runtime_error("[NetThreadManager::init_thread] Thread can't be null when destroy_thread is called");
    Logger::get().log("INFO", "[NetThreadManager::destroy_thread] Stopping network thread");
    destroyThread = true;
    t->join();
    t = nullptr;
}

void NetThreadManager::go_to_background() {
    std::scoped_lock l{backgroundUpdateMutex};
    backgroundUpdate = true;
}

void NetThreadManager::go_to_foreground() {
    std::scoped_lock l{backgroundUpdateMutex};
    backgroundUpdate = false;
}

void NetThreadManager::destroy() {
    if(!started)
        return;
    started = false;
#ifdef __ANDROID__
    AndroidJNICalls::stopNetworkService();
#else
    NetLibrary::destroy_websocket();
#endif
}
