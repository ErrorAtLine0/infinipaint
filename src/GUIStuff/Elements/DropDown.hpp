#pragma once
#include "Element.hpp"
#include "SelectableButton.hpp"
#include "../ElementHelpers/ScrollAreaHelpers.hpp"
#include "../ElementHelpers/LayoutHelpers.hpp"
#include "../ElementHelpers/TextLabelHelpers.hpp"
#include "../ElementHelpers/ButtonHelpers.hpp"
#include "SVGIcon.hpp"

namespace GUIStuff {

struct DropdownOptions {
    float width = 200.0f;
    std::function<void()> onClick;
};

template <typename T> class DropDown : public Element {
    public:
        DropDown(GUIManager& gui): Element(gui) {}
        void layout(const Clay_ElementId& id, T* data, const std::vector<std::string>& selections, const DropdownOptions& options = {}) {
            opts = options;
            d = data;
            ElementHelpers::left_to_right_layout(gui, CLAY_SIZING_FIXED(opts.width), CLAY_SIZING_FIT(0), [&] {
                gui.element<SelectableButton>("dropdown", SelectableButton::Data{
                    .drawType = SelectableButton::DrawType::FILLED,
                    .isSelected = isOpen,
                    .onClick = [&] {
                        isOpen = !isOpen;
                        gui.set_to_layout();
                    },
                    .innerContent = [&](const SelectableButton::InnerContentCallbackParameters&) {
                        CLAY_AUTO_ID({
                            .layout = {
                                .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0) },
                                .padding = {.left = 4, .right = 4},
                                .childGap = gui.io.theme->childGap1,
                                .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                                .layoutDirection = CLAY_LEFT_TO_RIGHT,
                            }
                        }) {
                            ElementHelpers::text_label(gui, selections[*data]);
                            CLAY_AUTO_ID({.layout = {.sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)}}}) {}
                            CLAY_AUTO_ID({
                                .layout = {
                                    .sizing = {.width = CLAY_SIZING_FIT(ElementHelpers::SMALL_BUTTON_SIZE), .height = CLAY_SIZING_FIT(ElementHelpers::SMALL_BUTTON_SIZE)}
                                }
                            }) {
                                gui.element<SVGIcon>("dropico", "data/icons/droparrow.svg", isOpen);
                            }
                        }
                    }
                });

                if(isOpen) {
                    gui.set_z_index(gui.get_z_index() + 1, [&] {
                        gui.element<LayoutElement>("DROPDOWN", [&](LayoutElement*, const Clay_ElementId& lId) {
                            Clay_ElementData dropdownElemData = Clay_GetElementData(lId);
                            float calculatedDropdownMaxHeight = 0.0f;
                            if(dropdownElemData.found)
                                calculatedDropdownMaxHeight = std::max(gui.io.windowSize.y() - dropdownElemData.boundingBox.y - 2.0f, 0.0f);
                            CLAY(lId, {
                                .layout = {
                                    .sizing = {.width = CLAY_SIZING_FIXED(opts.width), .height = CLAY_SIZING_FIT(0, calculatedDropdownMaxHeight)},
                                    .childGap = 0
                                },
                                .backgroundColor = convert_vec4<Clay_Color>(gui.io.theme->backColor1),
                                .cornerRadius = CLAY_CORNER_RADIUS(4),
                                .floating = {
                                    .offset = {
                                        .y = 4
                                    },
                                    .zIndex = gui.get_z_index(),
                                    .attachPoints = {
                                        .element = CLAY_ATTACH_POINT_LEFT_TOP,
                                        .parent = CLAY_ATTACH_POINT_LEFT_BOTTOM
                                    },
                                    .attachTo = CLAY_ATTACH_TO_PARENT
                                },
                                .border = {
                                    .color = convert_vec4<Clay_Color>(gui.io.theme->fillColor2),
                                    .width = CLAY_BORDER_OUTSIDE(1)
                                }
                            }) {
                                ElementHelpers::scroll_area_many_entries(gui, "dropdown scroll area", {
                                    .entryHeight = 18.0f,
                                    .entryCount = selections.size(),
                                    .clipHorizontal = true,
                                    .elementContent = [&](size_t i) {
                                        bool selectedEntry = static_cast<size_t>(*data) == i;
                                        gui.element<LayoutElement>("elem", [&] (LayoutElement*, const Clay_ElementId& lId) {
                                            CLAY(lId, {
                                                .layout = {
                                                    .sizing = {.width = CLAY_SIZING_FIXED(250), .height = CLAY_SIZING_FIXED(18)},
                                                    .padding = CLAY_PADDING_ALL(0),
                                                    .childGap = 0,
                                                    .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER},
                                                    .layoutDirection = CLAY_TOP_TO_BOTTOM
                                                }
                                            }) {
                                                SkColor4f entryColor;
                                                if(selectedEntry)
                                                    entryColor = gui.io.theme->backColor2;
                                                else if(hoveringOver.has_value() && hoveringOver.value() == i)
                                                    entryColor = gui.io.theme->backColor2;
                                                else
                                                    entryColor = gui.io.theme->backColor1;
                                                CLAY_AUTO_ID({
                                                    .layout = {
                                                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                                                        .padding = {.left = 2, .right = 2, .top = 0, .bottom = 0},
                                                        .childGap = 0,
                                                        .childAlignment = {.x = CLAY_ALIGN_X_LEFT, .y = CLAY_ALIGN_Y_CENTER}
                                                    },
                                                    .backgroundColor = convert_vec4<Clay_Color>(entryColor)
                                                }) {
                                                    ElementHelpers::text_label(gui, selections[i]);
                                                }
                                            }
                                        }, LayoutElement::Callbacks {
                                            .mouseButton = [&, i](LayoutElement* l, const InputManager::MouseButtonCallbackArgs& button) {
                                                update_hovering_over(i, l->mouseHovering);
                                                if(l->mouseHovering && button.button == InputManager::MouseButton::LEFT && button.down) {
                                                    gui.set_post_callback_func([&, i] {
                                                        *d = i;
                                                        isOpen = false;
                                                        opts.onClick();
                                                    });
                                                    gui.set_to_layout();
                                                }
                                            },
                                            .mouseMotion = [&, i](LayoutElement* l, const InputManager::MouseMotionCallbackArgs& motion) {
                                                update_hovering_over(i, l->mouseHovering);
                                            }
                                        });
                                    }
                                });
                            }
                        });
                    });
                }
            });
        }
    private:
        void update_hovering_over(size_t i, bool mouseHovering) {
            auto oldHoveringOver = hoveringOver;
            if(mouseHovering)
                hoveringOver = i;
            else if(hoveringOver.has_value() && hoveringOver.value() == i)
                hoveringOver = std::nullopt;
            if(oldHoveringOver != hoveringOver)
                gui.set_to_layout();
        }
        T* d;
        DropdownOptions opts;
        bool isOpen = false;
        std::optional<size_t> hoveringOver;
};

}
