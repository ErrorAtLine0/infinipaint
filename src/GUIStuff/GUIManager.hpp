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

        void new_id(const char* id, const std::function<void()>& f);
        void new_id(int64_t id, const std::function<void()>& f);

        void set_z_index(int16_t z, const std::function<void()>& f);
        int16_t get_z_index();

        void set_post_callback_func(const std::function<void()>& f);
        void set_post_callback_func_high_priority(const std::function<void()>& f);
        void run_post_callback_func();

        template <typename ElementType, typename... Args> ElementType* element(const char* id, const Args&... a) {
            push_id(id);

            Element* oldParent = parentElement;
            ElementType* elem = insert_element<ElementType>();
            elem->parent = parentElement;
            parentElement = elem;

            elem->set_parent_clipping_region(clippingRegion);
            Clay_ElementId clayId = strArena.elem_id_from_id_stack(idStack);
            elem->layout(clayId, a...);
            elem->set_bounding_box_from_elem_data(Clay_GetElementData(clayId)); // Setting bounding box after layout ensures that the element will have its bounding box set if it's set to be drawn.

            parentElement = oldParent;

            pop_id();
            return elem;
        }

        template <typename ElementType, typename... Args> ElementType* clipping_element(const char* id, const Args&... a) {
            push_id(id);

            Element* oldParent = parentElement;
            ElementType* elem = insert_element<ElementType>();
            elem->parent = parentElement;
            parentElement = elem;

            elem->set_parent_clipping_region(clippingRegion);
            auto oldClippingRegion = clippingRegion;
            if(elem->get_bb().has_value()) {
                if(clippingRegion.has_value())
                    clippingRegion = clippingRegion.value().get_intersection_between_aabbs(elem->get_bb().value());
                else
                    clippingRegion = elem->get_bb();
            }

            Clay_ElementId clayId = strArena.elem_id_from_id_stack(idStack);
            elem->layout(clayId, a...);
            elem->set_bounding_box_from_elem_data(Clay_GetElementData(clayId));

            clippingRegion = oldClippingRegion;

            parentElement = oldParent;

            pop_id();
            return elem;
        }

        DefaultStringArena strArena;

        void input_key_callback(const InputManager::KeyCallbackArgs& key);
        void input_mouse_button_callback(InputManager::MouseButtonCallbackArgs button);
        void input_mouse_motion_callback(InputManager::MouseMotionCallbackArgs motion);
        void input_mouse_wheel_callback(InputManager::MouseWheelCallbackArgs wheel);

        bool cursor_obstructed() const;
    private:
        void layout();
        void layout_begin();
        void layout_end();
        void single_layout_run();

        void mouse_callback(const Vector2f& mousePos, const std::function<void(ElementContainer*)>& f);

        GUIManagerIDStack idStack;
        std::vector<ElementContainer*> orderedElements;
        std::unordered_map<GUIManagerIDStack, ElementContainer> elements;

        template <typename NewElement> NewElement* insert_element() {
            auto [it, inserted] = elements.emplace(idStack, ElementContainer());
            auto& container = it->second;
            if(inserted)
                container = {std::make_unique<NewElement>(*this)};
            container.elem->zIndex = zIndex;
            orderedElements.emplace_back(&container);
            container.isUsedThisFrame = true;
            return static_cast<NewElement*>(it->second.elem.get());
        }

        static void clay_error_handler(Clay_ErrorData errorData);
        static Clay_Dimensions clay_skia_measure_text(Clay_StringSlice str, Clay_TextElementConfig* config, void* userData);

        void push_id(int64_t id);
        void push_id(const char* id);
        void pop_id();

        int16_t zIndex = 0;
        Element* parentElement = nullptr;
        std::optional<SCollision::AABB<float>> clippingRegion;

        std::function<void()> postCallbackFunc;
        bool postCallbackFuncIsHighPriority;
        Clay_Context* clayInstance;
        Clay_Arena clayArena;
        Clay_RenderCommandArray renderCommands;
        bool setToLayout;
        bool cursorObstructed;
};

}
