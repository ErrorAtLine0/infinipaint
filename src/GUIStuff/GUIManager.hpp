#pragma once
#include <include/core/SkCanvas.h>
#include <Eigen/Dense>
#include "Elements/Element.hpp"
#include "Elements/NumberSlider.hpp"
#include "Elements/CheckBox.hpp"
#include "Elements/PaintCircleMenu.hpp"
#include "Elements/RadioButton.hpp"
#include "Elements/ColorPicker.hpp"
#include "Elements/SelectableButton.hpp"
#include "Elements/TextBox.hpp"
#include "Elements/TreeListing.hpp"
#include <any>
#include "GUIManagerID.hpp"
#include <filesystem>
#include <type_traits>
#include <clay.h>

#include <modules/skparagraph/include/Paragraph.h>

using namespace Eigen;

namespace GUIStuff {

class GUIManager {
    public:
        constexpr static float BIG_BUTTON_SIZE = 30;
        constexpr static float SMALL_BUTTON_SIZE = 20;

        struct ElementContainer {
            std::unique_ptr<Element> elem;
            std::any extra;
            bool isUsedThisFrame;
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

        bool input_text_field(const char* id, std::string_view name, std::string* val, bool updateEveryEdit = true, const std::function<void(SelectionHelper&)>& elemUpdate = nullptr);

        void text_paragraph(const char* id, std::unique_ptr<skia::textlayout::Paragraph> paragraph, float width, const std::function<void()>& elemUpdate = nullptr);
        void text_label_color(std::string_view val, const SkColor4f& color);
        void text_label_size(std::string_view val, float modifier);
        void text_label_light(std::string_view val);
        void text_label(std::string_view val);
        void text_label_light_centered(std::string_view val);
        void text_label_centered(std::string_view val);
        bool text_button_left_transparent(const char* id, std::string_view text, bool isSelected = false, const std::function<void()>& elemUpdate = nullptr);
        bool text_button(const char* id, std::string_view text, bool isSelected = false, const std::function<void()>& elemUpdate = nullptr);
        bool text_button_wide(const char* id, std::string_view text, bool isSelected = false, const std::function<void()>& elemUpdate = nullptr);
        bool text_button_sized(const char* id, std::string_view text, Clay_SizingAxis x, Clay_SizingAxis y, bool isSelected = false, const std::function<void()>& elemUpdate = nullptr);

        void svg_icon(const char* id, const std::string& svgPath, bool isHighlighted = false, const std::function<void()>& elemUpdate = nullptr);
        bool svg_icon_button(const char* id, const std::string& svgPath, bool isSelected = false, float size = BIG_BUTTON_SIZE, bool hasBorder = true, const std::function<void()>& elemUpdate = nullptr);

        bool svg_icon_button_transparent(const char* id, const std::string& svgPath, bool isSelected = false, float size = BIG_BUTTON_SIZE, bool hasBorder = true, const std::function<void()>& elemUpdate = nullptr);

        bool rotate_wheel(const char* id, double* angle, float size = BIG_BUTTON_SIZE, const std::function<void()>& elemUpdate = nullptr);

        void top_to_bottom_window_popup_layout(const Clay_ElementId& localID, Clay_SizingAxis x, Clay_SizingAxis y, const std::function<void()>& elemUpdate);
        void left_to_right_layout(Clay_SizingAxis x, Clay_SizingAxis y, const std::function<void()>& elemUpdate);
        void left_to_right_line_layout(const std::function<void()>& elemUpdate);
        void left_to_right_line_centered_layout(const std::function<void()>& elemUpdate);

        bool selectable_button(const char* id, const std::function<void(SelectionHelper&, bool)>& elemUpdate, GUIStuff::SelectableButton::DrawType drawType, bool isSelected);

        void push_id(int64_t id);
        void push_id(const char* id);
        void pop_id();

        void scroll_bar_area(const char* id, bool clipHorizontal, const std::function<void(float scrollContentHeight, float containerHeight, float& scrollAmount)>& elemUpdate, bool maxScrollBugWorkaround = true);
        void scroll_bar_many_entries_area(const char* id, float entryHeight, size_t entryCount, bool clipHorizontal, const std::function<void(size_t elementIndex, bool listHovered)>& entryUpdate, const std::function<void(float scrollContentHeight, float containerHeight, float& scrollAmount)>& elemUpdate = nullptr);

        bool input_color_component_255(const char* id, float* val, const std::function<void(SelectionHelper&)>& elemUpdate = nullptr);
        bool input_text(const char* id, std::string* val, bool updateEveryEdit = true, const std::function<void(SelectionHelper&)>& elemUpdate = nullptr);

        bool radio_button(const char* id, bool val, const std::function<void()>& elemUpdate = nullptr);
        bool radio_button_field(const char* id, std::string_view name, bool val, const std::function<void()>& elemUpdate = nullptr);

        void checkbox_field(const char* id, std::string_view name, bool* val, const std::function<void()>& elemUpdate = nullptr);
        void checkbox(const char* id, bool* val, const std::function<void()>& elemUpdate = nullptr);

        void tab_list(const char* id, const std::vector<std::pair<std::string, std::string>>& tabNames, size_t& selectedTab, std::optional<size_t>& closedTab, const std::function<void()>& elemUpdate = nullptr);

        void input_path(const char* id, std::filesystem::path* val, std::filesystem::file_type fileTypeRestriction, const std::function<void(SelectionHelper&)>& elemUpdate = nullptr);
        void input_path_field(const char* id, std::string_view name, std::filesystem::path* val, std::filesystem::file_type fileTypeRestriction, const std::function<void(SelectionHelper&)>& elemUpdate = nullptr);

        void dropdown_select(const char* id, size_t* val, const std::vector<std::string>& selections, float width = 200.0f, const std::function<void()>& hoverboxElemUpdate = nullptr);

        void paint_circle_popup_menu(const char* id, const Vector2f& centerPos, const PaintCircleMenu::Data& val, const std::function<void()>& elemUpdate = nullptr);
        void list_popup_menu(const char* id, Vector2f popupPos, const std::function<void()>& elemUpdate);

        void tree_listing(const char* id, NetworkingObjects::NetObjID rootObjID, const TreeListing::DisplayData& displayData, TreeListing::SelectionData& selectionData);

        void obstructing_window(const Clay_ElementId& elementID);

        template <typename NewElement> NewElement* insert_element() {
            auto [it, inserted] = elements.emplace(idStack, ElementContainer());
            auto& container = it->second;
            if(inserted)
                container = {std::make_unique<NewElement>()};
            container.isUsedThisFrame = true;
            return static_cast<NewElement*>(it->second.elem.get());
        }

        template <typename T> T& insert_any(const T& def) {
            auto [it, inserted] = elements.emplace(idStack, ElementContainer());
            auto& container = it->second;
            if(inserted)
                it->second.extra = def;
            container.isUsedThisFrame = true;
            return std::any_cast<T&>(it->second.extra);
        }

        template <typename T> T& insert_any_with_id(int64_t id, const T& def) {
            push_id(id);
            T& toRet = insert_any(def);
            pop_id();
            return toRet;
        }

        template <typename T> T& insert_any_with_function(std::function<T()> funcToRun) {
            auto [it, inserted] = elements.emplace(idStack, ElementContainer());
            auto& container = it->second;
            if(inserted)
                it->second.extra = funcToRun();
            container.isUsedThisFrame = true;
            return std::any_cast<T&>(it->second.extra);
        }

        template <typename T> T& insert_any_with_id_with_function(int64_t id, std::function<T()> funcToRun) {
            push_id(id);
            T& toRet = insert_any_with_function(funcToRun);
            pop_id();
            return toRet;
        }
        
        template <typename T> bool input_generic(const char* id, T* val, const std::function<std::optional<T>(const std::string&)>& fromStr, const std::function<std::string(const T&)>& toStr, bool singleLine, bool updateEveryEdit, const std::function<void(SelectionHelper&)>& elemUpdate = nullptr) {
            bool isUpdating = false;
            push_id(id);
            isUpdating = insert_element<TextBox<T>>()->update(*io, val, fromStr, toStr, singleLine, updateEveryEdit, elemUpdate);
            pop_id();
            return isUpdating;
        }

        template <typename T> bool input_scalar(const char* id, T* val, T min, T max, int decimalPrecision = 0, const std::function<void(SelectionHelper&)>& elemUpdate = nullptr) {
            return input_generic<T>(id, val, 
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

        bool input_scalar(const char* id, uint8_t* val, uint8_t min, uint8_t max, int decimalPrecision, const std::function<void(SelectionHelper&)>& elemUpdate);

        template <typename TContainer, typename T> bool input_scalar_fields(const char* id, std::string_view name, TContainer* val, size_t elemCount, T min, T max, int decimalPrecision = 0, const std::function<void(SelectionHelper&)>& elemUpdate = nullptr) {
            bool isUpdating = false;
            push_id(id);
            left_to_right_line_layout([&]() {
                text_label(name);
                for(size_t i = 0; i < elemCount; i++) {
                    push_id(i);
                    isUpdating |= input_scalar<T>("field", &(*val)[i], min, max, decimalPrecision, elemUpdate);
                    pop_id();
                }
            });
            pop_id();
            return isUpdating;
        }

        template <typename T> bool input_color_hex(const char* id, T* val, bool selectAlpha, const std::function<void(SelectionHelper&)>& elemUpdate = nullptr) {
            return input_generic<T>(id, val, 
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

        template <typename T> bool color_picker(const char* id, T* val, bool selectAlpha, const std::function<void()>& elemUpdate = nullptr) {
            push_id(id);
            bool isUpdating = insert_element<ColorPicker<T>>()->update(*io, val, selectAlpha, elemUpdate);
            pop_id();
            return isUpdating;
        }

        template <typename T> bool slider_scalar(const char* id, T* val, T min, T max, const std::function<void()>& elemUpdate = nullptr) {
            push_id(id);
            bool isUpdating = insert_element<NumberSlider<T>>()->update(*io, val, min, max, elemUpdate); 
            pop_id();
            return isUpdating;
        }

        template <typename T> bool input_scalar_field(const char* id, std::string_view name, T* val, T min, T max, int decimalPrecision = 0, const std::function<void(SelectionHelper&)>& elemUpdate = nullptr) {
            bool isUpdating = false;
            left_to_right_line_layout([&]() {
                text_label(name);
                isUpdating = input_scalar(id, val, min, max, decimalPrecision, elemUpdate);
            });
            return isUpdating;
        }

        template <typename T> bool slider_scalar_field(const char* id, std::string_view name, T* val, T min, T max, int decimalPrecision = 0, const std::function<void()>& elemUpdate = nullptr) {
            bool isUpdating = false;
            push_id(id);
            left_to_right_line_layout([&]() {
                text_label(name);
                isUpdating |= input_scalar("0", val, min, max, decimalPrecision, nullptr);
            });
            isUpdating |= slider_scalar("1", val, min, max, elemUpdate);
            pop_id();
            return isUpdating;
        }

        template <typename T> bool big_color_button(const char* id, T* val, bool isSelected = false, const std::function<void()>& elemUpdate = nullptr) {
            bool toRet = false;
            CLAY_AUTO_ID({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(GUIStuff::GUIManager::BIG_BUTTON_SIZE), .height = CLAY_SIZING_FIXED(GUIStuff::GUIManager::BIG_BUTTON_SIZE)}}}) {
                toRet = color_button<T>(id, val, isSelected, elemUpdate);
            }
            return toRet;
        }

        template <typename T> bool color_button(const char* id, T* val, bool isSelected = false, const std::function<void()>& elemUpdate = nullptr) {
            return selectable_button(id, [&](SelectionHelper& s, bool iS) {
                CLAY_AUTO_ID({.layout = { 
                        .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                    },
                    .backgroundColor = convert_vec4<Clay_Color>(*val),
                    .cornerRadius = CLAY_CORNER_RADIUS(5),
                }) {}
                if(elemUpdate)
                    elemUpdate();
            }, GUIStuff::SelectableButton::DrawType::TRANSPARENT_BORDER, isSelected);
        }

        template <typename T> bool color_button_big(const char* id, T* val, bool isSelected = false, const std::function<void()>& elemUpdate = nullptr) {
            bool toRet = false;
            CLAY_AUTO_ID({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(BIG_BUTTON_SIZE), .height = CLAY_SIZING_FIXED(BIG_BUTTON_SIZE)}}}) {
                toRet = selectable_button(id, [&](SelectionHelper& s, bool iS) {
                    CLAY_AUTO_ID({.layout = { 
                            .sizing = {.width = CLAY_SIZING_GROW(0), .height = CLAY_SIZING_GROW(0)},
                        },
                        .backgroundColor = convert_vec4<Clay_Color>(*val),
                        .cornerRadius = CLAY_CORNER_RADIUS(5),
                    }) {}
                    if(elemUpdate)
                        elemUpdate();
                }, GUIStuff::SelectableButton::DrawType::TRANSPARENT_BORDER, isSelected);
            }
            return toRet;
        }

        template <typename T> bool color_picker_button(const char* id, T* val, bool selectAlpha, const std::function<void()>& elemUpdate = nullptr) {
            bool isUpdating = false;
            push_id(id);
            bool& isOpen = insert_any_with_id<bool>(0, false);
            bool clicked;
            CLAY_AUTO_ID({.layout = {.sizing = {.width = CLAY_SIZING_FIXED(BIG_BUTTON_SIZE), .height = CLAY_SIZING_FIXED(BIG_BUTTON_SIZE) } } }) {
                clicked = color_button("0", val, isOpen, elemUpdate);
                if(clicked)
                    isOpen = true;
                if(isOpen) {
                    top_to_bottom_window_popup_layout(CLAY_ID_LOCAL("COLOR PICKER"), CLAY_SIZING_FIT(300), CLAY_SIZING_FIT(0), [&]() {
                        isUpdating = color_picker_items("c", val, selectAlpha);
                        if(io->mouse.leftClick && !Clay_Hovered() && !clicked)
                            isOpen = false;
                    });
                }
            }
            pop_id();
            return isUpdating;
        }

        bool font_picker(const char* id, std::string* fontName);

        template <typename T> bool color_picker_items(const char* id, T* val, bool selectAlpha, float fixedPickerWidth = 0.0f) {
            bool isUpdating = false;
            push_id(id);
            if(fixedPickerWidth == 0.0f)
                isUpdating |= color_picker("c", val, selectAlpha);
            else {
                CLAY_AUTO_ID({
                    .layout = {
                        .sizing = {.width = CLAY_SIZING_FIXED(fixedPickerWidth), .height = CLAY_SIZING_FIXED(fixedPickerWidth)}
                    }
                }) {
                    isUpdating |= color_picker("c", val, selectAlpha);
                }
            }
            left_to_right_line_layout([&]() {
                text_label("R");
                isUpdating |= input_color_component_255("r", &(*val)[0]);
                text_label("G");
                isUpdating |= input_color_component_255("g", &(*val)[1]);
                text_label("B");
                isUpdating |= input_color_component_255("b", &(*val)[2]);
                if(selectAlpha) {
                    text_label("A");
                    isUpdating |= input_color_component_255("a", &(*val)[3]);
                }
            });
            left_to_right_line_layout([&]() {
                text_label("Hex");
                isUpdating |= input_color_hex("h", val, selectAlpha);
            });
            pop_id();
            return isUpdating;
        }

        template <typename T> bool color_picker_button_field(const char* id, std::string_view name, T* val, bool selectAlpha, const std::function<void()>& elemUpdate = nullptr) {
            bool isUpdating = false;
            left_to_right_line_layout([&]() {
                isUpdating = color_picker_button(id, val, selectAlpha, elemUpdate);
                text_label(name);
            });
            return isUpdating;
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
