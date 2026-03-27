#pragma once
#include "../GUIManager.hpp"

namespace GUIStuff { namespace ElementHelpers {

void checkbox_boolean(GUIManager& gui, const char* id, bool* d, const std::function<void()>& onClick = nullptr);
void checkbox_field(GUIManager& gui, const char* id, std::string_view name, const std::function<bool()>& isTicked, const std::function<void()>& onClick);
void checkbox_boolean_field(GUIManager& gui, const char* id, std::string_view name, bool* d, const std::function<void()>& onClick = nullptr);

}}
