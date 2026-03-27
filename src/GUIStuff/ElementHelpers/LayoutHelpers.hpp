#pragma once
#include "../GUIManager.hpp"
#include "../Elements/LayoutElement.hpp"

namespace GUIStuff { namespace ElementHelpers {

void top_to_bottom_window_popup_layout(GUIManager& gui, const char* id, Clay_SizingAxis x, Clay_SizingAxis y, const std::function<void()>& innerContent, const LayoutElement::Callbacks& callbacks);
void left_to_right_layout(GUIManager& gui, Clay_SizingAxis x, Clay_SizingAxis y, const std::function<void()>& innerContent);
void left_to_right_line_layout(GUIManager& gui, const std::function<void()>& innerContent);
void left_to_right_line_centered_layout(GUIManager& gui, const std::function<void()>& innerContent);

}}
