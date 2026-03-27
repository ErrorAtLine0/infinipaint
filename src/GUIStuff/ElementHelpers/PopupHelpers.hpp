#pragma once
#include "../GUIManager.hpp"
#include "../Elements/PaintCircleMenu.hpp"

namespace GUIStuff { namespace ElementHelpers {

void paint_circle_popup_menu(GUIManager& gui, const char* id, const Vector2f& centerPos, const PaintCircleMenu::Data& val);

}}
