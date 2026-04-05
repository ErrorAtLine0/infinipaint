#include "MovableTabList.hpp"
#include "Helpers/ConvertVec.hpp"
#include "../GUIManager.hpp"
#include <iostream>

#include "../ElementHelpers/ButtonHelpers.hpp"
#include "../ElementHelpers/TextLabelHelpers.hpp"
#include "SVGIcon.hpp"

namespace GUIStuff {

using namespace ElementHelpers;

MovableTabList::MovableTabList(GUIManager& gui): Element(gui) {}

void MovableTabList::layout(const Clay_ElementId& id, const MovableTabListData& newData) {
    d = newData;
    CLAY(id, {.layout = { 
            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_FIXED(BIG_BUTTON_SIZE)},
            .padding = CLAY_PADDING_ALL(0),
            .childGap = 4,
            .childAlignment = { .x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER },
            .layoutDirection = CLAY_LEFT_TO_RIGHT
        },
        .clip = {.horizontal = true, .childOffset = Clay_GetScrollOffset()}
    }) {
        for(size_t i = 0; i < d.tabNames.size(); i++) {
            CLAY_AUTO_ID({.layout = { 
                    .sizing = {.width = CLAY_SIZING_FIT(200), .height = CLAY_SIZING_GROW(0)},
                }
            }) {
                gui.new_id(i, [&] {
                    gui.element<SelectableButton>("b", SelectableButton::Data{
                        .drawType = SelectableButton::DrawType::FILLED,
                        .isSelected = d.selectedTab == i,
                        .onClick = [&, i] {
                            if(d.selectedTab != i && d.changeSelectedTab) d.changeSelectedTab(i);
                        },
                        .innerContent = [&] (const SelectableButton::InnerContentCallbackParameters&) {
                            CLAY_AUTO_ID({.layout = { 
                                      .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                                      .padding = CLAY_PADDING_ALL(0),
                                      .childGap = 4,
                                      .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                                      .layoutDirection = CLAY_LEFT_TO_RIGHT 
                                  }
                            }) {
                                if(!d.tabNames[i].iconPath.empty()) {
                                    CLAY_AUTO_ID({.layout = { 
                                              .sizing = {.width = CLAY_SIZING_FIXED(SMALL_BUTTON_SIZE), .height = CLAY_SIZING_FIXED(SMALL_BUTTON_SIZE)},
                                              .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                                              .layoutDirection = CLAY_LEFT_TO_RIGHT 
                                          }
                                    }) {
                                        gui.element<SVGIcon>("icon", d.tabNames[i].iconPath);
                                    }
                                }
                                text_label(gui, d.tabNames[i].name);
                                CLAY_AUTO_ID({.layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}}) {}
                                CLAY_AUTO_ID({.layout = { 
                                          .sizing = {.width = CLAY_SIZING_FIXED(SMALL_BUTTON_SIZE), .height = CLAY_SIZING_FIXED(SMALL_BUTTON_SIZE)},
                                          .childAlignment = { .x = CLAY_ALIGN_X_CENTER, .y = CLAY_ALIGN_Y_CENTER },
                                          .layoutDirection = CLAY_LEFT_TO_RIGHT 
                                      }
                                }) {
                                    gui.element<SelectableButton>("close button", SelectableButton::Data {
                                        .drawType = SelectableButton::DrawType::TRANSPARENT_ALL,
                                        .onClick = [&, i] {
                                            if(d.closeTab) d.closeTab(i);
                                        },
                                        .innerContent = [&] (const SelectableButton::InnerContentCallbackParameters&) {
                                            gui.element<SVGIcon>("close", "data/icons/close.svg");
                                        }
                                    });
                                }
                            }
                        }
                    });
                });
            }
        }
    }
}

}
