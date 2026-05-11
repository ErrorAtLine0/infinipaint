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
#include <functional>
#include <vector>

template <typename... Args> class CallbackManager {
    public:
        typedef std::function<void(const Args&...)> Callback;
        Callback* register_callback(const Callback& func) {
            Callback* toRet = new Callback(func);
            funcs.emplace_back(toRet);
            return toRet;
        }
        void deregister_callback(Callback* callbackToRemove) {
            if(std::erase(funcs, callbackToRemove) > 0)
                delete callbackToRemove;
        }
        void run_callbacks(const Args&... a) {
            for(auto& f : funcs)
                (*f)(a...);
        }
        void clear() {
            for(Callback* c : funcs)
                delete c;
            funcs.clear();
        }
        ~CallbackManager() {
            clear();
        }
    private:
        std::vector<Callback*> funcs;
};
