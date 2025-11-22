#include "FontPicker.hpp"
#include "Helpers/ConvertVec.hpp"
#include <algorithm>
#include <modules/skparagraph/include/DartTypes.h>
#include <modules/skparagraph/include/ParagraphStyle.h>
#include <modules/skparagraph/src/ParagraphBuilderImpl.h>
#include <modules/skunicode/include/SkUnicode_icu.h>
#include <Helpers/Random.hpp>

bool GUIStuff::FontPicker::update(UpdateInputData& io, std::string* fontName, GUIManager* gui) {
    if(sortedFontList.empty()) {
        auto icu = SkUnicodes::ICU::Make();
        std::set<std::string> sortedFontSet;

        if(io.fonts->localFontMgr) {
            for(int i = 0; i < io.fonts->localFontMgr->countFamilies(); i++) {
                SkString familyNameSkString;
                io.fonts->localFontMgr->getFamilyName(i, &familyNameSkString);
                if(familyNameSkString.isEmpty())
                    continue;
                std::string familyName(familyNameSkString.c_str(), familyNameSkString.size());
                sortedFontSet.emplace(familyName);
            }
        }
        for(int i = 0; i < io.fonts->defaultFontMgr->countFamilies(); i++) {
            SkString familyNameSkString;
            io.fonts->defaultFontMgr->getFamilyName(i, &familyNameSkString);
            if(familyNameSkString.isEmpty())
                continue;
            std::string familyName(familyNameSkString.c_str(), familyNameSkString.size());
            sortedFontSet.emplace(familyName);
        }
        sortedFontList = std::vector<std::string>(sortedFontSet.begin(), sortedFontSet.end());
        sortedFontListLowercase = sortedFontList;
        for(std::string& s : sortedFontListLowercase)
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    }

    if(!fontName)
        return false;

    bool fontChangedFromTextbox = false;
    bool fontChangedFromSelection = false;

    bool clickedOut = io.mouse.leftClick;

    const float ENTRY_HEIGHT = 25.0f;

    CLAY({
        .layout = {
            .sizing = {.width = CLAY_SIZING_FIXED(250), .height = CLAY_SIZING_FIT(0)},
            .childGap = 4
        }
    }) {
        fontChangedFromTextbox = gui->input_text("Font textbox", fontName, true, [&](SelectionHelper& s) {
            if(s.selected) {
                dropdownOpen = true;
                clickedOut = false;
            }
        });

        auto it = std::find(sortedFontList.begin(), sortedFontList.end(), *fontName);
        size_t val = std::numeric_limits<size_t>::max();
        if(it == sortedFontList.end()) {
            std::string lowerFamilyName = *fontName;
            std::transform(lowerFamilyName.begin(), lowerFamilyName.end(), lowerFamilyName.begin(), ::tolower);
            auto itLower = std::find(sortedFontListLowercase.begin(), sortedFontListLowercase.end(), lowerFamilyName);
            if(itLower != sortedFontListLowercase.end()) {
                val = itLower - sortedFontListLowercase.begin();
                *fontName = sortedFontList[val];
            }
        }
        else
            val = it - sortedFontList.begin();

        if(gui->svg_icon_button("Open font dropdown", "data/icons/droparrow.svg", dropdownOpen, GUIManager::SMALL_BUTTON_SIZE)) {
            dropdownOpen = !dropdownOpen;
            clickedOut = false;
        }
        if(dropdownOpen) {
            CLAY({
                .layout = {
                    .sizing = {.width = CLAY_SIZING_FIXED(250), .height = CLAY_SIZING_FIT(0, 300)},
                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                },
                .backgroundColor = convert_vec4<Clay_Color>(io.theme->backColor1),
                .cornerRadius = CLAY_CORNER_RADIUS(4),
                .floating = {
                    .offset = {
                        .y = 2
                    },
                    .attachPoints = {
                        .element = CLAY_ATTACH_POINT_LEFT_TOP,
                        .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM
                    },
                    .attachTo = CLAY_ATTACH_TO_PARENT
                },
                .border = {
                    .color = convert_vec4<Clay_Color>(io.theme->fillColor2),
                    .width = CLAY_BORDER_OUTSIDE(1)
                }
            }) {
                gui->obstructing_window();
                if(Clay_Hovered())
                    clickedOut = false;
                gui->scroll_bar_many_entries_area("Font Selector", ENTRY_HEIGHT, sortedFontList.size(), true, [fontName, gui, &fontChangedFromSelection, &sortedFontList = sortedFontList, val, &io = io](size_t i, bool listHovered) {
                    bool selectedEntry = val == i;
        
                    skia::textlayout::ParagraphStyle pStyle;
                    pStyle.setTextAlign(skia::textlayout::TextAlign::kLeft);
                    pStyle.setTextHeightBehavior(skia::textlayout::kDisableAll);
        
                    skia::textlayout::StrutStyle strutStyle;
                    strutStyle.setStrutEnabled(true);
                    strutStyle.setForceStrutHeight(true);
                    pStyle.setStrutStyle(strutStyle);
        
                    skia::textlayout::TextStyle tStyle;
                    tStyle.setFontSize(io.fontSize);
                    tStyle.setFontFamilies({SkString{sortedFontList[i].c_str()}, SkString{"Roboto"}});
                    tStyle.setForegroundColor(SkPaint{io.theme->frontColor1});
                    pStyle.setTextStyle(tStyle);
        
                    skia::textlayout::ParagraphBuilderImpl pBuilder(pStyle, io.fonts->collection, SkUnicodes::ICU::Make());
                    tStyle.setFontStyle(SkFontStyle::Normal());
        
                    pBuilder.pushStyle(tStyle);
                    pBuilder.addText(sortedFontList[i].c_str(), sortedFontList[i].size());
        
                    auto p = pBuilder.Build();
        
                    CLAY({
                        .layout = {
                            .sizing = {.width = CLAY_SIZING_FIXED(250), .height = CLAY_SIZING_FIXED(25)},
                            .childGap = 1,
                            .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_TOP},
                            .layoutDirection = CLAY_TOP_TO_BOTTOM
                        }
                    }) {
                        SkColor4f entryColor;
                        if(selectedEntry)
                            entryColor = io.theme->backColor2;
                        else if(Clay_Hovered())
                            entryColor = io.theme->backColor2;
                        else
                            entryColor = io.theme->backColor1;
                        CLAY({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(25)},
                                .padding = {.left = 2, .right = 2},
                                .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                            },
                            .backgroundColor = convert_vec4<Clay_Color>(entryColor)
                        }) {
                            gui->text_paragraph("font name", std::move(p), 10000.0f); // Setting width to numeric_limits::max will lead to multiplying it by any number greater than 1 to = infinity, which will cause issues
        
                            if(io.mouse.leftClick && Clay_Hovered()) {
                                *fontName = sortedFontList[i];
                                fontChangedFromSelection = true;
                            }
                        }
                    }
                }, [ENTRY_HEIGHT, fontChangedFromTextbox, io = &io, val](float scrollContentHeight, float containerHeight, float& scrollAmount) {
                    if(fontChangedFromTextbox && !io->mouse.leftClick && val != std::numeric_limits<size_t>::max())
                        scrollAmount = -static_cast<float>(val) * ENTRY_HEIGHT;
                });
                #ifdef __EMSCRIPTEN__
                    CLAY({
                        .layout = {
                            .padding = CLAY_PADDING_ALL(io.theme->padding1)
                    }}) {
                        gui->text_label_light_centered("Local fonts not usable in web version");
                    }
                #endif
            }
        }
    }

    if(clickedOut)
        dropdownOpen = false;
    
    return fontChangedFromSelection | fontChangedFromTextbox;
}

void GUIStuff::FontPicker::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) {
}
