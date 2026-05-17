/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once
#include "DrawingProgramScreen.hpp"
#include "../Toolbar.hpp"

class DesktopDrawingProgramScreen : public DrawingProgramScreen {
    public:
        DesktopDrawingProgramScreen(MainProgram& m);
        virtual void update() override;
        virtual void gui_layout_run() override;
        virtual bool app_close_requested() override;
        virtual void input_key_callback(const InputManager::KeyCallbackArgs& key) override;
        virtual void input_text_key_callback(const InputManager::KeyCallbackArgs& key) override;
        virtual void open_file_selector(const std::string& filePickerName, const std::vector<ExtensionFilter>& extensionFilters, OpenFileSelectorCallback postSelectionFunc, const std::string& fileName = "", bool isSaving = false) override;
        virtual void on_tab_close() override;
        virtual float calculate_gui_scale() override;

    private:
        Toolbar toolbar;
};
