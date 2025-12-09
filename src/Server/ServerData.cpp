#include "ServerData.hpp"
#include "../DrawComponents/DrawComponent.hpp"

void ServerData::save(cereal::PortableBinaryOutputArchive& a) const {
    a(canvasBackColor, canvasScale);
    a((uint64_t)components.size());
    for(uint64_t i = 0; i < components.size(); i++)
        a(components[i]->get_type(), components[i]->id, components[i]->coords, *components[i]);
    a(grids);
}

void ServerData::load(cereal::PortableBinaryInputArchive& a) {
    a(canvasBackColor, canvasScale);
    uint64_t compCount;
    a(compCount);
    for(uint64_t i = 0; i < compCount; i++) {
        DrawComponentType t;
        a(t);
        auto newComp = DrawComponent::allocate_comp_type(t);
        a(newComp->id, newComp->coords, *newComp);
        components.emplace_back(newComp);
        idToComponentMap.emplace(newComp->id, newComp);
    }
    a(grids);
}

void ServerData::scale_up(const WorldScalar& scaleUpAmount) {
    for(auto& [n, g] : grids)
        g.scale_up(scaleUpAmount);
    for(auto& c : components)
        c->scale_up(scaleUpAmount);
}
