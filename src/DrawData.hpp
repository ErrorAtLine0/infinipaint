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
    bool dontUseDrawProgCache = false;
    bool clampDrawBetween = true;
    WorldScalar clampDrawMinimum;
    WorldScalar clampDrawMaximum;
    WorldScalar mipMapLevelOne;
    WorldScalar mipMapLevelTwo;
    WorldScalar mipMapLevelThree;
    WorldScalar mipMapLevelFour;
    WorldScalar mipMapLevelFive;
    void refresh_draw_optimizing_values();
};
