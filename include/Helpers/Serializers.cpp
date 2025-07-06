#include "Serializers.hpp"

void to_json(nlohmann::json& j, const SkColor4f& d) {
    j = nlohmann::json::array();
    j.emplace_back(d.fR);
    j.emplace_back(d.fG);
    j.emplace_back(d.fB);
    j.emplace_back(d.fA);
}

void from_json(const nlohmann::json& j, SkColor4f& d) {
    j.at(0).template get_to<float>(d.fR);
    j.at(1).template get_to<float>(d.fG);
    j.at(2).template get_to<float>(d.fB);
    j.at(3).template get_to<float>(d.fA);
}
