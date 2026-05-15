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
#include <thread>
#include <mutex>
#include <functional>

class MainProgram;

class NetThreadManager {
    public:
        static NetThreadManager& get();
        void init(MainProgram* m);
        void synchronous_update();
        void thread_update();
        void destroy();
        void init_thread();
        void destroy_thread();
        void go_to_background();
        void go_to_foreground();
    private:
        static NetThreadManager global;
        MainProgram* main;
        std::mutex backgroundUpdateMutex;
        bool backgroundUpdate = false;
        std::unique_ptr<std::thread> t;
        std::atomic<bool> destroyThread = false;

        bool started = false;
};
