#pragma once
#include "../GUIManager.hpp"

namespace GUIStuff { namespace ElementHelpers {

void radio_button_field(GUIManager& gui, const char* id, std::string_view name, const std::function<bool()>& isTicked, const std::function<void()>& onClick);

template <typename T> void radio_button_selector(GUIManager& gui, const char* id, T* data, const std::vector<std::pair<std::string_view, T>>& options, const std::function<void()>& onClick = nullptr) {
    gui.new_id(id, [&] {
        for(size_t i = 0; i < options.size(); i++) {
            gui.new_id(i, [&] {
                auto& option = options[i];
                radio_button_field(gui, "Option", option.first, [data, optionVal = option.second]{ return *data == optionVal; }, [data, optionVal = option.second, onClick]{ *data = optionVal; if(onClick) onClick(); });
            });
        }
    });
}

}}
