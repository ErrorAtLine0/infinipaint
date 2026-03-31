#pragma once
#include <include/core/SkCanvas.h>
#include <Eigen/Dense>
#include "GUIManagerID.hpp"
#include <clay.h>
#include "Elements/Element.hpp"

using namespace Eigen;

namespace GUIStuff {

class GUIManager {
    public:
        struct ElementContainer {
            std::unique_ptr<Element> elem;
            bool isUsedThisFrame;
        };

        GUIManager();
        void draw(SkCanvas* canvas, bool skiaAA);
        void set_to_layout();
        void layout_if_necessary();

        void update_window(const Vector2f& windowPos, const Vector2f& windowSize, float guiScaleMultiplier);

        UpdateInputData io;

        GUIManagerIDStack idStack;
        std::unordered_map<GUIManagerIDStack, ElementContainer> elements;

        void new_id(const char* id, const std::function<void()>& f);
        void new_id(int64_t id, const std::function<void()>& f);

        void set_post_callback_func(const std::function<void()>& f);

        int16_t zIndex = 0;
        template <typename ElementType, typename... Args> ElementType* element(const char* id, const Args&... a) {
            push_id(id);
            ElementType* elem = insert_element<ElementType>();
            elem->layout(a...);
            pop_id();
            return elem;
        }

        DefaultStringArena strArena;

    private:
        void layout();
        void update_element_bounding_boxes();
        void layout_begin();
        void layout_end();
        void single_layout_run();

        template <typename NewElement> NewElement* insert_element() {
            auto [it, inserted] = elements.emplace(idStack, ElementContainer());
            auto& container = it->second;
            if(inserted)
                container = {std::make_unique<NewElement>(*this)};
            container.elem->zIndex = zIndex;
            container.isUsedThisFrame = true;
            return static_cast<NewElement*>(it->second.elem.get());
        }

        static void clay_error_handler(Clay_ErrorData errorData);
        static Clay_Dimensions clay_skia_measure_text(Clay_StringSlice str, Clay_TextElementConfig* config, void* userData);

        void push_id(int64_t id);
        void push_id(const char* id);
        void pop_id();

        std::function<void()> postCallbackFunc;
        Clay_Context* clayInstance;
        Clay_Arena clayArena;
        Clay_RenderCommandArray renderCommands;
        bool setToLayout;
};

}
