#include "DelayUpdateSerializedClassManager.hpp"

namespace NetworkingObjects {
    void DelayUpdateSerializedClassManager::update() {
        std::erase_if(updatingObjs, [](auto& updatingPair) {
            if((std::chrono::steady_clock::now() - updatingPair.second.lastUpdateTimePoint) >= std::chrono::seconds(3)) {
                updatingPair.second.setDataAfterTimeout();
                return true;
            }
            return false;
        });
    }
}
