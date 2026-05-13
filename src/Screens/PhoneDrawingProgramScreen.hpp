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
#include "../GUIStuff/Elements/TreeListing.hpp"

class PhoneDrawingProgramScreen : public DrawingProgramScreen {
    public:
        PhoneDrawingProgramScreen(MainProgram& m);
        virtual void update() override;
        virtual void gui_layout_run() override;
        virtual void input_global_back_button_callback() override;
        virtual void input_app_about_to_go_to_background_callback() override;
        struct ColorSelectorData {
            std::function<void()> onChange;
            std::function<void()> onSelect;
            std::function<void()> onDeselect;
        };
        struct ColorSelectorButtonData {
            std::function<void()> onSelectorButtonClick;
            std::function<void()> onChange;
            std::function<void()> onSelect;
            std::function<void()> onDeselect;
        };
        struct ColorPickerPopupData {
            enum class ScreenType {
                NORMAL,
                EXTRA,
                PALETTES
            } screenType = ScreenType::NORMAL;
            size_t selectedPalette = 0;
            std::string newPaletteStr;
            std::set<GUIStuff::TreeListingObjIndexList> paletteListSelection;
        } colorPickerPopupData;
        void color_selector(Vector4f* color, const ColorSelectorData& colorSelectorData = {});
        void color_selector_button(const char* id, Vector4f* color, const ColorSelectorButtonData& colorSelectorData = {});
        void color_settings_popup(Vector4f* color, const ColorSelectorData& d, std::function<void()> backOnPaletteButton = nullptr, bool setAlphaToOne = false, bool setToTransparentCancelButton = false);
        void reset_color_picker_popup_data();
    private:
        std::filesystem::path fileOldPath;
        struct TopToolbarRemainingAreaButton {
            std::string name;
            std::string svgPath;
            bool isSelected = false;
            std::function<void()> onClick;
        };
        enum class SettingsMenuPopup {
            NONE,
            SETTINGS,
            COLOR_CHANGE
        } settingsMenuPopup = SettingsMenuPopup::NONE;

        enum class TopToolbarSettingsPopup {
            NONE,
            HIDDEN_BUTTONS,
            BOOKMARKS,
            LAYERS,
            GRIDS
        } topToolbarSettingsPopup = TopToolbarSettingsPopup::NONE;

        Vector4f backgroundColorTemporary;
        Vector4f* colorPickerPtr = nullptr;
        ColorSelectorData colorPickerData;

        void center_message(const char* id, const std::string& m);
        void default_bottom_toolbar();
        void bottom_toolbar_gui();
        void bottom_extra_toolbar_gui();
        void save_files();
        void top_toolbar();
        void top_toolbar_settings_popup();
        void top_toolbar_remaining_area();
        void top_toolbar_hidden_button_popup(GUIStuff::Element* b, std::vector<TopToolbarRemainingAreaButton> l);
        void bottom_toolbar();
        void main_display();
        void tool_settings_popup();
};
