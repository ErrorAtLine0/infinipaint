#pragma once
#include "Helpers/ConvertVec.hpp"
#include <include/core/SkCanvas.h>
#include "../../TimePoint.hpp"
#include <Eigen/Dense>
#include <include/core/SkFont.h>
#include <include/core/SkFontMgr.h>
#include <include/core/SkTypeface.h>
#include <modules/skparagraph/include/FontCollection.h>
#include <modules/svg/include/SkSVGDOM.h>
#include <Helpers/SCollision.hpp>
#include <Helpers/Serializers.hpp>
#include <nlohmann/json.hpp>
#include "../../RichText/TextBox.hpp"

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
    SkColor4f fillColor1 = {0.8f, 0.785f, 1.0f, 1.0f};
    SkColor4f fillColor2 = {0.3f, 0.3f, 0.477f, 1.0f};
    SkColor4f backColor1 = {0.0f, 0.0f, 0.0f, 1.0f};
    SkColor4f backColor2 = {1.0f, 1.0f, 1.0f, 1.0f};
    SkColor4f frontColor1 = {1.0f, 1.0f, 1.0f, 1.0f};
    SkColor4f frontColor2 = {0.9f, 0.9f, 0.9f, 1.0f};

    SkColor4f errorColor = {1.0f, 0.0f, 0.0f, 1.0f};
    SkColor4f warningColor = {1.0f, 1.0f, 0.0f, 1.0f};

    float hoverExpandTime = 0.1f;
    uint16_t childGap1 = 10;
    uint16_t padding1 = 8;
    float windowCorners1 = 8;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Theme, fillColor1, fillColor2, backColor1, backColor2, frontColor1, frontColor2, errorColor, warningColor, hoverExpandTime, childGap1, padding1, windowCorners1)
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

    std::function<void(std::shared_ptr<RichText::TextBox>, std::shared_ptr<RichText::TextBox::Cursor>)> richTextBoxEdit;

    SkFont get_font(float fSize) const;

    uint16_t fontSize = 16;

    sk_sp<skia::textlayout::FontCollection> fontCollection;

    std::string textInput;
    bool hoverObstructed = false;
    bool acceptingTextInput = false;
    float deltaTime = 0.0f;

    sk_sp<SkFontMgr> textFontMgrLocal;
    sk_sp<SkFontMgr> textFontMgrFallback;
    sk_sp<SkTypeface> textTypeface;

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
