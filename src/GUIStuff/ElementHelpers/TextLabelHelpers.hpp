#pragma once
#include "../GUIManager.hpp"

namespace GUIStuff { namespace ElementHelpers {

void text_label_color(GUIManager& gui, std::string_view val, const SkColor4f& color);
void text_label_size(GUIManager& gui, std::string_view val, float modifier);
void text_label_light(GUIManager& gui, std::string_view val);
void text_label(GUIManager& gui, std::string_view val);
void text_label_light_centered(GUIManager& gui, std::string_view val);
void text_label_centered(GUIManager& gui, std::string_view val);

}}
