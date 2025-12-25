#include "SerializedBlendMode.hpp"

SkBlendMode serialized_blend_mode_to_sk_blend_mode(SerializedBlendMode serializedBlendMode) {
    int skBlendModeStartVal = static_cast<int>(SkBlendMode::kClear);
    return static_cast<SkBlendMode>(skBlendModeStartVal + static_cast<uint8_t>(serializedBlendMode));
}

const std::vector<std::string>& get_blend_mode_name_list() {
    static std::vector<std::string> blendModeList;
    if(blendModeList.empty()) {
        for(uint8_t i = 0; i < static_cast<uint8_t>(SerializedBlendMode::LAST_MODE); i++)
            blendModeList.emplace_back(nlohmann::json(static_cast<SerializedBlendMode>(i)));
    }
    return blendModeList;
}
