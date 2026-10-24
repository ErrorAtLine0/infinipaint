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
#include "CanvasEditorUI/CanvasEditorTopToolbar.hpp"
#include "CanvasScreen.hpp"
#include "../GUIStuff/Elements/TreeListing.hpp"

class CanvasEditorScreen : public CanvasScreen {
    public:
        CanvasEditorScreen(MainProgram& m, bool initMobileUI);
        ~CanvasEditorScreen();
        virtual void update() override;
        virtual void gui_layout_run() override;
        virtual bool input_keybind_callback(const Vector2ui32& newKey) override;
        virtual void input_global_back_button_callback() override;
        virtual void input_app_about_to_go_to_background_callback() override;
        virtual void on_tab_close() override;
        virtual SCollision::AABB<float> canvas_safe_area() override;
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
        enum class SettingsMenuPopup {
            NONE, // No settings open
            SETTINGS, // Tool settings open. If colorPickerPtr isn't null, it'll show that, but the back button can be pressed to go back to settings
            COLOR_CHANGE // Settings only meant to change color. No back button
        } settingsMenuPopup = SettingsMenuPopup::NONE;
        void set_settings_menu_popup_mode_with_color_picker_data(SettingsMenuPopup newSettingsMode, Vector4f* newColorPickerPtr = nullptr, const ColorSelectorData& initColorPickerData = ColorSelectorData());
        void set_color_picker_data(Vector4f* newColorPickerPtr, const ColorSelectorData& initColorPickerData);

        Vector4f backgroundColorTemporary;
        Vector4f* colorPickerPtr = nullptr;
        ColorSelectorData colorPickerData;

        void gui_layout_safety_checks();
        void hidden_gui();
        void center_message(const char* id, const std::string& m);
        void default_bottom_toolbar();
        void bottom_toolbar_gui();
        void bottom_extra_toolbar_gui();
        void bottom_toolbar();
        void main_display();
        void tool_settings_popup();
        void desktop_save_file();
        void desktop_save_as_file();
        void save_to_file();
        void open_world_file(bool isClient, const std::string& netSource, const std::string& serverLocalID2);

        CanvasEditorTopToolbar topToolbar;

        const bool mobileUI;

        friend class CanvasEditorTopToolbar;
};
