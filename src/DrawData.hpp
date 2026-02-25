#pragma once
#include "DrawCamera.hpp"
#include "ResourceManager.hpp"
#include "FontData.hpp"

using namespace Eigen;

class Toolbar;
class MainProgram;

struct DrawData {
    DrawCamera cam;
    ResourceManager* rMan;
    MainProgram* main;
    bool takingScreenshot = false;
    bool isSVGRender = false;
    bool clampDrawBetween = true;
    bool drawGrids = true;
    bool transparentBackground = false;
    WorldScalar clampDrawMinimum;
    WorldScalar mipMapLevelOne;
    WorldScalar mipMapLevelTwo;
    void refresh_draw_optimizing_values();
};
