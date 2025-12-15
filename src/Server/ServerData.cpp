#include "ServerData.hpp"

void ServerData::save(cereal::PortableBinaryOutputArchive& a) const {
    a(canvasBackColor, canvasScale);
}

void ServerData::load(cereal::PortableBinaryInputArchive& a) {
    a(canvasBackColor, canvasScale);
}

void ServerData::scale_up(const WorldScalar& scaleUpAmount) {
}
