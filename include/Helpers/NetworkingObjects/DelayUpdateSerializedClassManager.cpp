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

#include "DelayUpdateSerializedClassManager.hpp"
#include <chrono>

namespace NetworkingObjects {
    void DelayUpdateSerializedClassManager::update(NetworkingObjects::NetObjManager& netObjMan) {
        for(auto& [objID, updatingObj] : updatingObjs) {
            if(updatingObj.lastTemporaryUpdateTimePoint.has_value() && (std::chrono::steady_clock::now() - updatingObj.lastTemporaryUpdateTimePoint.value()) >= std::chrono::milliseconds(300)) {
                updatingObj.sendFinalUpdate();
                updatingObj.lastTemporaryUpdateTimePoint = std::nullopt;
            }
        }
        std::erase_if(updatingObjs, [](auto& updatingPair) {
            if((std::chrono::steady_clock::now() - updatingPair.second.lastUpdateTimePoint) >= std::chrono::seconds(3) && !updatingPair.second.lastTemporaryUpdateTimePoint.has_value()) {
                updatingPair.second.setDataAfterTimeout();
                return true;
            }
            return false;
        });
    }
}
