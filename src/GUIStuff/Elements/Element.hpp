#pragma once
#include "Helpers/ConvertVec.hpp"
#include <include/core/SkCanvas.h>
#include "../../TimePoint.hpp"
#include <Eigen/Dense>
#include <include/core/SkFont.h>
#include <include/core/SkFontMgr.h>
#include <modules/svg/include/SkSVGDOM.h>
#include <Helpers/SCollision.hpp>
#include <Helpers/Serializers.hpp>
#include <nlohmann/json.hpp>

using namespace Eigen;

namespace GUIStuff {

// Taken from https://github.com/TimothyHoytBSME/ClayMan (rewritten to be a separate struct, and use templates to change size)
template <size_t S> class StringArena {
    public:
        StringArena() {
            stringArena = std::vector<char>(S);
        }
        void reset() {
            stringArenaIndex = 0;
        }
        const char* insert_string_into_arena(const std::string& str) {
            size_t strSize = str.size();
            if(stringArenaIndex + strSize + 1 > S)
                throw std::overflow_error("[StringArena::insert_string_into_arena] Not enough space to insert string");
            char* startPtr = &stringArena[stringArenaIndex];
            for(size_t i = 0; i < strSize; i++)
                stringArena[stringArenaIndex++] = str[i];
            stringArena[stringArenaIndex++] = ' ';
            return startPtr;
        }
        Clay_String std_str_to_clay_str(const std::string& str) {
            return Clay_String{.length = static_cast<int32_t>(str.size()), .chars = insert_string_into_arena(str)};
        }
    private:
        std::vector<char> stringArena;
        size_t stringArenaIndex;
};

typedef StringArena<4000000> DefaultStringArena;

SkFont get_setup_skfont();

struct Theme {
    SkColor4f fillColor1 = {0.5f, 0.6f, 1.0f, 1.0f};
    SkColor4f fillColor2 = {0.4f, 0.5f, 0.9f, 1.0f};
    SkColor4f fillColor3 = {1.0f, 0.0f, 0.0f, 1.0f};
    SkColor4f fillColor4 = {1.0f, 1.0f, 0.0f, 1.0f};
    SkColor4f fillColor5 = {0.7f, 0.7f, 1.0f, 1.0f};
    SkColor4f backColor1 = {1.0f, 1.0f, 1.0f, 0.9f};
    SkColor4f backColor2 = {0.95f, 0.95f, 0.95f, 1.0f};
    SkColor4f backColor3 = {0.9f, 0.9f, 0.9f, 1.0f};
    SkColor4f frontColor1 = {0.0f, 0.0f, 0.0f, 1.0f};
    SkColor4f frontColor2 = {0.1f, 0.1f, 0.1f, 1.0f};

    float hoverExpandTime = 0.1f;
    uint16_t childGap1 = 10;
    uint16_t padding1 = 10;
    float windowCorners1 = 10;
    uint16_t windowBorders1 = 2;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Theme, fillColor1, fillColor2, fillColor3, fillColor4, fillColor5, backColor1, backColor2, backColor3, frontColor1, frontColor2, hoverExpandTime, childGap1, padding1, windowCorners1, windowBorders1)
};

std::shared_ptr<Theme> get_default_dark_mode();

struct UpdateInputData {
    struct {
        int leftClick = 0;
        int rightClick = 0;
        bool leftDoubleClick = false;
        bool leftHeld = false;
        bool rightHeld = false;
        Vector2f globalPos{0, 0};
        Vector2f pos{0, 0}; // Position relative to window 
        Vector2f scroll;
    } mouse;

    struct {
        bool left = false;
        bool right = false;
        bool up = false;
        bool down = false;
        bool leftShift = false;
        bool leftCtrl = false;
        bool home = false;
        bool del = false;
        bool backspace = false;
        bool enter = false;
        bool selectAll = false;
        bool copy = false;
        bool paste = false;
        bool cut = false;
        bool escape = false;
    } key;

    struct {
        std::function<std::string()> textInFunc;
        std::optional<std::string> textOut;
    } clipboard;

    SkFont get_font(float fSize) const;

    uint16_t fontSize = 20;
    sk_sp<SkTypeface> textTypeface;

    std::string textInput;
    bool hoverObstructed = false;
    bool acceptingTextInput = false;
    float deltaTime = 0.0f;
    sk_sp<SkFontMgr> textFontMgr;
    DefaultStringArena* strArena;
    std::shared_ptr<Theme> theme;
    std::unordered_map<std::string, sk_sp<SkSVGDOM>> svgData;
};

class SelectionHelper {
    public:
        void update(bool isHovering, bool isLeftClick, bool isLeftHeld);
        bool held = false;
        bool hovered = false;
        bool selected = false;
        bool clicked = false;
        bool justUnselected = false;
};

class Element {
    public:
        virtual void clay_draw(SkCanvas* canvas, UpdateInputData& io, Clay_RenderCommand* command) = 0;
        virtual ~Element() = default;

    protected:
        static SCollision::AABB<float> get_bb(Clay_RenderCommand* command);
};

}
