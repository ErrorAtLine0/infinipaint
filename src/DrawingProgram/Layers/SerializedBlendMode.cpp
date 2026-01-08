#include "SerializedBlendMode.hpp"
#include <unordered_set>

SkBlendMode serialized_blend_mode_to_sk_blend_mode(SerializedBlendMode serializedBlendMode) {
    int skBlendModeStartVal = static_cast<int>(SkBlendMode::kClear);
    return static_cast<SkBlendMode>(skBlendModeStartVal + static_cast<uint8_t>(serializedBlendMode));
}

const std::vector<SerializedBlendMode>& get_blend_mode_useful_list() {
    static std::vector<SerializedBlendMode> blendModeList;
    if(blendModeList.empty()) {
        std::unordered_set<SerializedBlendMode> exclusionList = {SerializedBlendMode::CLEAR, SerializedBlendMode::SRC, SerializedBlendMode::DST};
        for(uint8_t i = 0; i < static_cast<uint8_t>(SerializedBlendMode::LAST_MODE); i++) {
            if(!exclusionList.contains(static_cast<SerializedBlendMode>(i)))
                blendModeList.emplace_back(static_cast<SerializedBlendMode>(i));
        }
    }
    return blendModeList;
}

const std::vector<std::string>& get_blend_mode_useful_name_list() {
    static std::vector<std::string> blendModeNameList;
    if(blendModeNameList.empty()) {
        for(SerializedBlendMode i : get_blend_mode_useful_list())
            blendModeNameList.emplace_back(nlohmann::json(static_cast<SerializedBlendMode>(i)));
    }
    return blendModeNameList;
}
