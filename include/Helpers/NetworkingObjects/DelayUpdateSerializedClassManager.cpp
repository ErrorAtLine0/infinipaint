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
