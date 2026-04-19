#include "FileSelectScreen.hpp"
#include "../MainProgram.hpp"
#include "../GUIHolder.hpp"
#include "../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "Helpers/ConvertVec.hpp"
#include "../GUIStuff/Elements/GridScrollArea.hpp"
#include "../GUIStuff/ElementHelpers/ButtonHelpers.hpp"
#include "../GUIStuff/ElementHelpers/ScrollAreaHelpers.hpp"

FileSelectScreen::FileSelectScreen(MainProgram& m): Screen(m) {}

void FileSelectScreen::gui_layout_run() {
    using namespace GUIStuff;
    using namespace ElementHelpers;
    auto& gui = main.g.gui;

    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(gui.io.windowSize.x()), .height = CLAY_SIZING_FIXED(gui.io.windowSize.y())},
            .childGap = 15,
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
    }) {
        text_label(gui, "Test grid");
        gui.element<GridScrollArea>("Test grid", GridScrollArea::Options{
            .entryMaximumWidth = 100.0f,
            .childAlignmentX = CLAY_ALIGN_X_LEFT,
            .entryHeight = 100.0f,
            .entryCount = 20000,
            .elementContent = [&](size_t i) {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { .width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                        .padding = CLAY_PADDING_ALL(2),
                        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                    }
                }) {
                    text_button_sized(gui, "button", "Square " + std::to_string(i), CLAY_SIZING_GROW(0), CLAY_SIZING_GROW(0));
                }
            }
        });
    }
}

void FileSelectScreen::draw(SkCanvas* canvas) {
    canvas->clear(SkColor4f{0.0f, 0.0f, 0.0f, 0.0f});
}
