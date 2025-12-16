#include "ServerData.hpp"

void ServerData::save(cereal::PortableBinaryOutputArchive& a) const {
    a(canvasScale);
}

void ServerData::load(cereal::PortableBinaryInputArchive& a) {
    a(canvasScale);
}

void ServerData::scale_up(const WorldScalar& scaleUpAmount) {
}
