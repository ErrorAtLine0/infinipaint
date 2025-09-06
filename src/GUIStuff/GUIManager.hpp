#pragma once
#include <include/core/SkCanvas.h>
#include <Eigen/Dense>
#include "Elements/Element.hpp"
#include "Elements/NumberSlider.hpp"
#include "Elements/CheckBox.hpp"
#include "Elements/PaintCircleMenu.hpp"
#include "Elements/RadioButton.hpp"
#include "Elements/ColorPicker.hpp"
#include "Elements/TextBox.hpp"
#include <any>
#include "GUIManagerID.hpp"
#include <filesystem>
#include <type_traits>
#include <clay.h>

using namespace Eigen;

namespace GUIStuff {

class GUIManager {
    public:
        struct ElementContainer {
            std::unique_ptr<Element> elem;
            std::any extra;
        };

        GUIManager();
        void begin();
        void end();
        void draw(SkCanvas* canvas);

        Vector2f windowPos = Vector2f{0.0f, 0.0f};
        Vector2f windowSize = Vector2f{0.0f, 0.0f};
        std::shared_ptr<UpdateInputData> io;

        GUIManagerIDStack idStack;
        std::unordered_map<GUIManagerIDStack, ElementContainer> elements;

        Vector2f screen_pos_to_window_pos(const Vector2f& screenPos);
        Vector2f get_mouse_pos();

        void input_text_field(const std::string& id, const std::string& name, std::string* val, const std::function<void()>& elemUpdate = nullptr);

        void text_label_color(const std::string& val, const SkColor4f& color);
        void text_label_size(const std::string& val, float modifier);
        void text_label(const std::string& val);
        void text_label_centered(const std::string& val);
        bool text_button(const std::string& id, const std::string& text, bool isSelected = false, const std::function<void()>& elemUpdate = nullptr);
        bool text_button_wide(const std::string& id, const std::string& text, bool isSelected = false, const std::function<void()>& elemUpdate = nullptr);
        bool text_button_sized(const std::string& id, const std::string& text, Clay_SizingAxis x, Clay_SizingAxis y, bool isSelected = false, const std::function<void()>& elemUpdate = nullptr);

        void svg_icon(const std::string& id, const std::string& svgPath, bool isHighlighted = false, const std::function<void()>& elemUpdate = nullptr);
        bool svg_icon_button(const std::string& id, const std::string& svgPath, bool isSelected = false, float size = 40.0f, const std::function<void()>& elemUpdate = nullptr);

        void top_to_bottom_window_popup_layout(Clay_SizingAxis x, Clay_SizingAxis y, const std::function<void()>& elemUpdate);
        void left_to_right_layout(Clay_SizingAxis x, Clay_SizingAxis y, const std::function<void()>& elemUpdate);
        void left_to_right_line_layout(const std::function<void()>& elemUpdate);

        bool selectable_button(const std::string& id, const std::function<void(SelectionHelper&, bool)>& elemUpdate, bool hasBorder, bool hasBorderPadding, bool isSelected);

        void push_id(int64_t id);
        void push_id(const std::string& id);
        void pop_id();

        void scroll_bar_area(const std::string& uniqueId, const std::function<void(float scrollContentHeight, float containerHeight, float scrollAmount)>& elemUpdate);
        void scroll_bar_many_entries_area(const std::string& uniqueId, float entryHeight, size_t entryCount, const std::function<void(size_t elementIndex, bool listHovered)>& entryUpdate, const std::function<void(float scrollContentHeight, float containerHeight, float scrollAmount)>& elemUpdate = nullptr);

        void input_color_component_255(const std::string& id, float* val, const std::function<void()>& elemUpdate = nullptr);
        void input_text(const std::string& id, std::string* val, const std::function<void()>& elemUpdate = nullptr);

        bool radio_button(const std::string& id, bool val, const std::function<void()>& elemUpdate = nullptr);
        bool radio_button_field(const std::string& id, const std::string& name, bool val, const std::function<void()>& elemUpdate = nullptr);

        void checkbox_field(const std::string& id, const std::string& name, bool* val, const std::function<void()>& elemUpdate = nullptr);
        void checkbox(const std::string& id, bool* val, const std::function<void()>& elemUpdate = nullptr);

        void tab_list(const std::string& id, const std::vector<std::pair<std::string, std::string>>& tabNames, size_t& selectedTab, std::optional<size_t>& closedTab, const std::function<void()>& elemUpdate = nullptr);

        void input_path(const std::string& id, std::filesystem::path* val, std::filesystem::file_type fileTypeRestriction, const std::function<void()>& elemUpdate = nullptr);
        void input_path_field(const std::string& id, const std::string& name, std::filesystem::path* val, std::filesystem::file_type fileTypeRestriction, const std::function<void()>& elemUpdate = nullptr);

        void dropdown_select(const std::string& id, size_t* val, const std::vector<std::string>& selections, float width = 200.0f, const std::function<void()>& hoverboxElemUpdate = nullptr);

        void paint_circle_popup_menu(const std::string& id, const Vector2f& centerPos, const PaintCircleMenu::Data& val, const std::function<void()>& elemUpdate = nullptr);

        void obstructing_window();

        template <typename NewElement> NewElement* insert_element() {
            auto [it, inserted] = elements.emplace(idStack, ElementContainer());
            if(inserted)
                it->second = {std::make_unique<NewElement>()};
            return static_cast<NewElement*>(it->second.elem.get());
        }

        template <typename T> T& insert_any(const T& def) {
            auto [it, inserted] = elements.emplace(idStack, ElementContainer());
            if(inserted)
                it->second.extra = def;
            return std::any_cast<T&>(it->second.extra);
        }

        template <typename T> T& insert_any_with_id(int64_t id, const T& def) {
            push_id(id);
            T& toRet = insert_any(def);
            pop_id();
            return toRet;
        }
        
        template <typename T> void input_generic(const std::string& id, T* val, const std::function<std::optional<T>(const std::string&)>& fromStr, const std::function<std::string(const T&)>& toStr, bool singleLine, bool updateEveryEdit, const std::function<void()>& elemUpdate = nullptr) {
            push_id(id);
            insert_element<TextBox<T>>()->update(*io, val, fromStr, toStr, singleLine, updateEveryEdit, elemUpdate);
            pop_id();
        }

        template <typename T> void input_scalar(const std::string& id, T* val, T min, T max, int decimalPrecision = 0, const std::function<void()>& elemUpdate = nullptr) {
            input_generic<T>(id, val, 
                [&](const std::string& a) {
                    if(a.empty())
                        return min;
                    T toRet;
                    std::stringstream ss;
                    ss << a;
                    ss >> toRet;
                    if(ss.fail())
                        return min;
                    return std::clamp(toRet, min, max);
                },
                [&](const T& a) {
                    std::stringstream ss;
                    if(std::is_floating_point<T>()) {
                        ss.precision(decimalPrecision);
                        ss << std::fixed << *val;
                    }
                    else
                        ss << *val;
                    return ss.str();
                }, true, false, elemUpdate);
        }

        void input_scalar(const std::string& id, uint8_t* val, uint8_t min, uint8_t max, int decimalPrecision, const std::function<void()>& elemUpdate);

        template <typename TContainer, typename T> void input_scalar_fields(const std::string& id, const std::string& name, TContainer* val, size_t elemCount, T min, T max, int decimalPrecision = 0, const std::function<void()>& elemUpdate = nullptr) {
            push_id(id);
            left_to_right_line_layout([&]() {
                text_label(name);
                for(size_t i = 0; i < elemCount; i++)
                    input_scalar<T>(std::to_string(i), &(*val)[i], min, max, decimalPrecision, elemUpdate);
            });
            pop_id();
        }

        template <typename T> void input_color_hex(const std::string& id, T* val, bool selectAlpha, const std::function<void()>& elemUpdate = nullptr) {
            input_generic<T>(id, val, 
                [&](const std::string& str) {
                    T def = {0.0f, 0.0f, 0.0f, 1.0f};
                    unsigned startIndex = 0;
                    if(str.empty())
                        return def;

                    if(str[0] == '#')
                        startIndex++;

                    for(unsigned i = 0; i < (selectAlpha ? 4 : 3); i++) {
                        int byteColor;
                        std::stringstream ss;
                        size_t startingAt = startIndex + i * 2;
                        if(startingAt + 1 >= str.size())
                            break;
                        ss << str.substr(startingAt, 2);
                        ss >> std::hex >> byteColor;
                        if(ss.fail())
                            break;
                        def[i] = byteColor / 255.0f;
                    }
                    return def;
                }, 
                [&](const T& color) {
                    std::stringstream ss;
                    ss << "#" << std::setfill('0') << std::setw(2) << std::hex;
                    for(size_t i = 0; i < (selectAlpha ? 4 : 3); i++) {
                        int convertTo255 = static_cast<int>(std::clamp<float>(color[i], 0, 1) * 255);
                        ss << std::setfill('0') << std::setw(2) << std::hex << convertTo255;
                    }
                    return ss.str();
                },
                true, false, elemUpdate);
        }

        template <typename T> void color_picker(const std::string& id, T* val, bool selectAlpha, const std::function<void()>& elemUpdate = nullptr) {
            push_id(id);
            insert_element<ColorPicker<T>>()->update(*io, val, selectAlpha, elemUpdate);
            pop_id();
        }

        template <typename T> void slider_scalar(const std::string& id, T* val, T min, T max, const std::function<void()>& elemUpdate = nullptr) {
            push_id(id);
            insert_element<NumberSlider<T>>()->update(*io, val, min, max, elemUpdate); 
            pop_id();
        }

        template <typename T> void input_scalar_field(const std::string& id, const std::string& name, T* val, T min, T max, int decimalPrecision = 0, const std::function<void()>& elemUpdate = nullptr) {
            left_to_right_line_layout([&]() {
                text_label(name);
                input_scalar(id, val, min, max, decimalPrecision, elemUpdate);
            });
        }

        template <typename T> void slider_scalar_field(const std::string& id, const std::string& name, T* val, T min, T max, int decimalPrecision = 0, const std::function<void()>& elemUpdate = nullptr) {
            push_id(id);
            left_to_right_line_layout([&]() {
                text_label(name);
                input_scalar("0", val, min, max, decimalPrecision, elemUpdate);
            });
            slider_scalar("1", val, min, max, elemUpdate);
            pop_id();
        }

        template <typename T> bool color_button(const std::string& id, T* val, bool isSelected = false, const std::function<void()>& elemUpdate = nullptr) {
            return selectable_button(id, [&](SelectionHelper& s, bool iS) {
                CLAY({.layout = { 
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                    },
                    .backgroundColor = convert_vec4<Clay_Color>(*val),
                    .cornerRadius = CLAY_CORNER_RADIUS(5),
                }) {}
                if(elemUpdate)
                    elemUpdate();
            }, true, false, isSelected);
        }

        template <typename T> void color_picker_button(const std::string& id, T* val, bool selectAlpha, const std::function<void()>& elemUpdate = nullptr) {
            push_id(id);
            bool& isOpen = insert_any_with_id<bool>(0, false);
            bool clicked;
            CLAY ({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(40), .height = CLAY_SIZING_FIXED(40) } } }) {
                clicked = color_button("0", val, isOpen, elemUpdate);
            }
            if(clicked)
                isOpen = true;
            if(isOpen) {
                top_to_bottom_window_popup_layout(CLAY_SIZING_FIT(300), CLAY_SIZING_FIT(0), [&]() {
                    color_picker_items("c", val, selectAlpha);
                    if(io->mouse.leftClick && !Clay_Hovered() && !clicked)
                        isOpen = false;
                });
            }
            pop_id();
        }

        template <typename T> void color_picker_items(const std::string& id, T* val, bool selectAlpha) {
            push_id(id);
            color_picker("c", val, selectAlpha);
            left_to_right_line_layout([&]() {
                text_label("R");
                input_color_component_255("r", &(*val)[0]);
                text_label("G");
                input_color_component_255("g", &(*val)[1]);
                text_label("B");
                input_color_component_255("b", &(*val)[2]);
                if(selectAlpha) {
                    text_label("A");
                    input_color_component_255("a", &(*val)[3]);
                }
            });
            left_to_right_line_layout([&]() {
                text_label("Hex");
                input_color_hex("h", val, selectAlpha);
            });
            pop_id();
        }

        template <typename T> void color_picker_button_field(const std::string& id, const std::string& name, T* val, bool selectAlpha, const std::function<void()>& elemUpdate = nullptr) {
            left_to_right_line_layout([&]() {
                color_picker_button(id, val, selectAlpha, elemUpdate);
                text_label(name);
            });
        }

    private:
        static void clay_error_handler(Clay_ErrorData errorData);
        static Clay_Dimensions clay_skia_measure_text(Clay_StringSlice str, Clay_TextElementConfig* config, void* userData);

        DefaultStringArena strArena;

        Clay_Context* clayInstance;
        Clay_Arena clayArena;
        Clay_RenderCommandArray renderCommands;
};

}
