#include "GridManager.hpp"
#include "Helpers/Networking/NetLibrary.hpp"
#include "Server/CommandList.hpp"
#include "SharedTypes.hpp"
#include "World.hpp"
#include "MainProgram.hpp"
#include <algorithm>

GridManager::GridManager(World& w):
    world(w) {}

void GridManager::init() {
    if(world.netObjMan.is_server())
        grids = world.netObjMan.make_obj<NetworkingObjects::NetObjOrderedList<WorldGrid>>();
}

void GridManager::add_default_grid(const std::string& newName) {
    if(grids) {
        WorldGrid g;
        g.name = newName;
        g.color = color_mul_alpha(convert_vec4<Vector4f>(world.canvasTheme.toolFrontColor), 0.4f);
        g.size = world.drawData.cam.c.inverseScale * WorldScalar(WorldGrid::GRID_UNIT_PIXEL_SIZE);
        g.offset = world.drawData.cam.c.pos + world.drawData.cam.c.dir_from_space(world.main.window.size.cast<float>() * 0.5f);
        grids->emplace_back_direct(grids, g);
    }
}

void GridManager::remove_grid(uint32_t indexToRemove) {
    if(grids)
        grids->erase(grids, indexToRemove);
}

void GridManager::draw_back(SkCanvas* canvas, const DrawData& drawData) {
    if(grids) {
        for(uint32_t i = 0; i < grids->size(); i++) {
            auto& g = grids->at(i)->obj;
            if(!g->displayInFront)
                g->draw(*this, canvas, drawData);
        }
    }
}

void GridManager::draw_front(SkCanvas* canvas, const DrawData& drawData) {
    if(grids) {
        for(uint32_t i = 0; i < grids->size(); i++) {
            auto& g = grids->at(i)->obj;
            if(g->displayInFront)
                g->draw(*this, canvas, drawData);
        }
    }
}

void GridManager::draw_coordinates(SkCanvas* canvas, const DrawData& drawData) {
    if(grids) {
        Vector2f axisOffset{0.0f, 0.0f};
        for(uint32_t i = 0; i < grids->size(); i++) {
            auto& g = grids->at(i)->obj;
            if(g->coordinatesWillBeDrawn && g->coordinatesAxisOnBounds) {
                g->draw_coordinates(canvas, drawData, axisOffset);
                g->coordinatesWillBeDrawn = false;
            }
        }
        for(uint32_t i = 0; i < grids->size(); i++) {
            auto& g = grids->at(i)->obj;
            if(g->coordinatesWillBeDrawn && !g->coordinatesAxisOnBounds)
                g->draw_coordinates(canvas, drawData, axisOffset);
            g->coordinatesWillBeDrawn = false;
        }
    }
}

void GridManager::scale_up(const WorldScalar& scaleUpAmount) {
    if(grids) {
        for(uint32_t i = 0; i < grids->size(); i++)
            grids->at(i)->obj->scale_up(scaleUpAmount);
    }
}
