#include "FileSelectScreen.hpp"
#include "../MainProgram.hpp"
#include "../GUIHolder.hpp"
#include "../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"

FileSelectScreen::FileSelectScreen(MainProgram& m): Screen(m) {}

void FileSelectScreen::gui_layout_run() {
    using namespace GUIStuff;
    using namespace ElementHelpers;
    CLAY_AUTO_ID({
        .layout = {}
    }) {
        text_label(main.g.gui, "File select screen");
    }
}

void FileSelectScreen::draw(SkCanvas* canvas) {
    canvas->clear(SkColor4f{0.0f, 0.0f, 0.0f, 0.0f});
}
