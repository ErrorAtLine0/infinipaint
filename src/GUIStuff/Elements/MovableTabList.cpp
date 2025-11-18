#include "MovableTabList.hpp"
#include "Helpers/ConvertVec.hpp"
#include "../GUIManager.hpp"
#include <iostream>

namespace GUIStuff {

void MovableTabList::update(UpdateInputData& io, const std::vector<std::pair<std::string, std::string>>& tabNames, size_t& selectedTab, std::optional<size_t>& closedTab, const std::function<void()>& elemUpdate) {
    size_t currentSelectedTab = selectedTab;
    if(tabNames != oldTabNames) {
        buttons = std::vector<SelectableButton>(tabNames.size());
        closeButtons = std::vector<SelectableButton>(tabNames.size());
        closeIcons = std::vector<SVGIcon>(tabNames.size());
        tabIcons = std::vector<SVGIcon>(tabNames.size());
        oldTabNames = tabNames;
    }

    CLAY({.layout = { 
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(GUIStuff::GUIManager::BIG_BUTTON_SIZE)},
            .padding = CLAY_PADDING_ALL(0),
            .childGap = 4,
            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER },
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        },
        .clip = {.horizontal = true, .childOffset = Clay_GetScrollOffset()}
    }) {
        selection.update(Clay_Hovered(), io.mouse.leftClick, io.mouse.leftHeld);
        if(elemUpdate)
            elemUpdate();
        for(size_t i = 0; i < buttons.size(); i++) {
            CLAY({.layout = { 
                    .sizing = {.width = CLAY_SIZING_FIT(200), .height = CLAY_SIZING_GROW(0)},
                }
            }) {
                buttons[i].update(io, SelectableButton::DrawType::FILLED, [&](SelectionHelper& s, bool iS) {
                    CLAY({.layout = { 
                              .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                              .padding = CLAY_PADDING_ALL(0),
                              .childGap = 4,
                              .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                              .layoutDirection = CLAY_LEFT_TO_RIGHT 
                          }
                    }) {
                        if(!tabNames[i].first.empty()) {
                            CLAY({.layout = { 
                                      .sizing = {.width = CLAY_SIZING_FIXED(GUIStuff::GUIManager::SMALL_BUTTON_SIZE), .height = CLAY_SIZING_FIXED(GUIStuff::GUIManager::SMALL_BUTTON_SIZE)},
                                      .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                                      .layoutDirection = CLAY_LEFT_TO_RIGHT 
                                  }
                            }) {
                                tabIcons[i].update(io, tabNames[i].first, iS, nullptr);
                            }
                        }
                        CLAY_TEXT(io.strArena->std_str_to_clay_str(tabNames[i].second), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(io.theme->frontColor1), .fontSize = io.fontSize }));
                        if(s.clicked)
                            selectedTab = i;
                        CLAY({.layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}}) {}
                        CLAY({.layout = { 
                                  .sizing = {.width = CLAY_SIZING_FIXED(GUIStuff::GUIManager::SMALL_BUTTON_SIZE), .height = CLAY_SIZING_FIXED(GUIStuff::GUIManager::SMALL_BUTTON_SIZE)},
                                  .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                                  .layoutDirection = CLAY_LEFT_TO_RIGHT 
                              }
                        }) {
                            closeButtons[i].update(io, SelectableButton::DrawType::TRANSPARENT_ALL, [&](SelectionHelper& s, bool iS) {
                                if(s.clicked)
                                    closedTab = i;
                                closeIcons[i].update(io, "data/icons/close.svg", s.held || s.hovered, nullptr);
                            }, false);
                        }
                    }
                }, currentSelectedTab == i);
            }
        }
    }
}

void MovableTabList::clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) {
}

}
