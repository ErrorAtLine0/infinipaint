#include "FileSelectScreen.hpp"
#include "../MainProgram.hpp"
#include "../GUIHolder.hpp"
#include "../GUIStuff/ElementHelpers/TextLabelHelpers.hpp"
#include "Helpers/ConvertVec.hpp"
#include "../GUIStuff/Elements/GridScrollArea.hpp"
#include "../GUIStuff/Elements/ImageDisplay.hpp"
#include "../World.hpp"

FileSelectScreen::FileSelectScreen(MainProgram& m): Screen(m) {}

void FileSelectScreen::gui_layout_run() {
    using namespace GUIStuff;
    using namespace ElementHelpers;
    auto& gui = main.g.gui;

    CLAY_AUTO_ID({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(gui.io.windowSize.x()), .height = CLAY_SIZING_FIXED(gui.io.windowSize.y())},
            .padding = CLAY_PADDING_ALL(15),
            .childGap = 15,
            .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_TOP },
            .layoutDirection = CLAY_TOP_TO_BOTTOM,
        },
    }) {
        text_label(gui, "File selector");
        auto& fileList = get_file_list();
        gui.element<GridScrollArea>("File selector grid", GridScrollArea::Options{
            .entryMaximumWidth = 200.0f,
            .childAlignmentX = CLAY_ALIGN_X_LEFT,
            .entryHeight = 250.0f,
            .entryCount = fileList.size(),
            .elementContent = [&](size_t i) {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = { .width = CLAY_SIZING_FIXED(150), .height = CLAY_SIZING_GROW(0)},
                        .padding = CLAY_PADDING_ALL(gui.io.theme->padding1),
                        .childGap = 2,
                        .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                        .layoutDirection = CLAY_TOP_TO_BOTTOM
                    }
                }) {
                    CLAY_AUTO_ID({
                        .layout = { .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) }},
                        .aspectRatio = { .aspectRatio = 1.0f }
                    }) {
                        gui.element<ImageDisplay>("ico", main.conf.configPath / "saveThumbnails" / (fileList[i] + ".jpg"));
                    }
                    text_label(gui, fileList[i]);
                }
            }
        });
    }
}

const std::vector<std::string>& FileSelectScreen::get_file_list() {
    if(!fileListOptional.has_value()) {
        std::filesystem::path savePath = main.conf.configPath / "saves";
        fileListOptional = std::vector<std::string>();
        auto& fL = fileListOptional.value();
        int globCount;
        char** filesInPath = SDL_GlobDirectory(savePath.string().c_str(), "*", 0, &globCount);
        if(filesInPath) {
            for(int i = 0; i < globCount; i++) {
                std::filesystem::path p = filesInPath[i];
                std::string fExt = p.extension().string();
                if(fExt == "." + World::FILE_EXTENSION)
                    fL.emplace_back(p.stem().string());
            }
        }
    }
    return fileListOptional.value();
}

void FileSelectScreen::draw(SkCanvas* canvas) {
    canvas->clear(main.g.gui.io.theme->backColor1);
}
