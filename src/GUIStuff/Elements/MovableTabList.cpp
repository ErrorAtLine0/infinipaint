#include "MovableTabList.hpp"
#include "Helpers/ConvertVec.hpp"
#include "../GUIManager.hpp"
#include <iostream>

namespace GUIStuff {

MovableTabList::MovableTabList(GUIManager& gui): Element(gui) {}

void MovableTabList::layout(const Clay_ElementId& id, const Data& d) {
    //CLAY_AUTO_ID({.layout = { 
    //        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(GUIStuff::GUIManager::BIG_BUTTON_SIZE)},
    //        .padding = CLAY_PADDING_ALL(0),
    //        .childGap = 4,
    //        .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER },
    //        .layoutDirection = CLAY_LEFT_TO_RIGHT
    //    },
    //    .clip = {.horizontal = true, .childOffset = Clay_GetScrollOffset()}
    //}) {
    //    for(size_t i = 0; i < d.tabNames.size(); i++) {
    //        CLAY_AUTO_ID({.layout = { 
    //                .sizing = {.width = CLAY_SIZING_FIT(200), .height = CLAY_SIZING_GROW(0)},
    //            }
    //        }) {
    //            buttons[i].update(io, SelectableButton::DrawType::FILLED, [&](SelectionHelper& s, bool iS) {
    //                CLAY_AUTO_ID({.layout = { 
    //                          .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
    //                          .padding = CLAY_PADDING_ALL(0),
    //                          .childGap = 4,
    //                          .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
    //                          .layoutDirection = CLAY_LEFT_TO_RIGHT 
    //                      }
    //                }) {
    //                    if(!tabNames[i].first.empty()) {
    //                        CLAY_AUTO_ID({.layout = { 
    //                                  .sizing = {.width = CLAY_SIZING_FIXED(GUIStuff::GUIManager::SMALL_BUTTON_SIZE), .height = CLAY_SIZING_FIXED(GUIStuff::GUIManager::SMALL_BUTTON_SIZE)},
    //                                  .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
    //                                  .layoutDirection = CLAY_LEFT_TO_RIGHT 
    //                              }
    //                        }) {
    //                            tabIcons[i].update(io, tabNames[i].first, iS, nullptr);
    //                        }
    //                    }
    //                    CLAY_TEXT(io.strArena->std_str_to_clay_str(tabNames[i].second), CLAY_TEXT_CONFIG({.textColor = convert_vec4<Clay_Color>(io.theme->frontColor1), .fontSize = io.fontSize }));
    //                    if(s.clicked)
    //                        selectedTab = i;
    //                    CLAY_AUTO_ID({.layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}}) {}
    //                    CLAY_AUTO_ID({.layout = { 
    //                              .sizing = {.width = CLAY_SIZING_FIXED(GUIStuff::GUIManager::SMALL_BUTTON_SIZE), .height = CLAY_SIZING_FIXED(GUIStuff::GUIManager::SMALL_BUTTON_SIZE)},
    //                              .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
    //                              .layoutDirection = CLAY_LEFT_TO_RIGHT 
    //                          }
    //                    }) {
    //                        closeButtons[i].update(io, SelectableButton::DrawType::TRANSPARENT_ALL, [&](SelectionHelper& s, bool iS) {
    //                            if(s.clicked)
    //                                closedTab = i;
    //                            closeIcons[i].update(io, "data/icons/close.svg", s.held || s.hovered, nullptr);
    //                        }, false);
    //                    }
    //                }
    //            }, currentSelectedTab == i);
    //        }
    //    }
    //}
}

}
