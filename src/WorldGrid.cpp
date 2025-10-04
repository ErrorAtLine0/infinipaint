#include "WorldGrid.hpp"
#include "GridManager.hpp"
#include "Helpers/MathExtras.hpp"
#include "MainProgram.hpp"
#include <include/core/SkFontMetrics.h>

sk_sp<SkRuntimeEffect> WorldGrid::circlePointEffect;
sk_sp<SkRuntimeEffect> WorldGrid::squarePointEffect;
sk_sp<SkRuntimeEffect> WorldGrid::squareLinesEffect;
sk_sp<SkRuntimeEffect> WorldGrid::horizontalLinesEffect;
Vector2f WorldGrid::oldWindowSize = Vector2f{0.0f, 0.0f};
unsigned WorldGrid::GRID_UNIT_PIXEL_SIZE = 25;

// NOTE: Skia shaders return premultiplied alpha colors

const char* circlePointShaderCode = R"V(
uniform vec4 gridColor;
uniform float mainGridScale;
uniform vec2 mainGridClosestPoint;
uniform float mainGridPointSize;

uniform float divGridAlphaFrac;
uniform float divGridScale;
uniform vec2 divGridClosestPoint;
uniform float divGridPointSize;

float get_color_val_circle_point(float2 fCoord, float gScale, float gPointSize, float2 gClosestPoint) {
    float2 modCoord = gScale > 100000.0 ? fCoord - gClosestPoint + float2(0.5 * gScale) : mod(fCoord - gClosestPoint + float2(0.5 * gScale), float2(gScale));
    return 1.0 - smoothstep(gPointSize, gPointSize + 1.0, distance(modCoord, float2(0.5 * gScale)));
}

vec4 main(float2 fragCoord) {
    float mainGridColor = get_color_val_circle_point(fragCoord, mainGridScale, mainGridPointSize, mainGridClosestPoint);
    float divGridColor = get_color_val_circle_point(fragCoord, divGridScale, divGridPointSize, divGridClosestPoint) * divGridAlphaFrac;
    float fin = max(mainGridColor, divGridColor);

    vec4 pointColorUnmultiplied = vec4(gridColor.rgb, 1.0);

  	return pointColorUnmultiplied * gridColor.a * fin;
})V";

const char* squarePointShaderCode = R"V(
uniform vec4 gridColor;
uniform float mainGridScale;
uniform vec2 mainGridClosestPoint;
uniform float mainGridPointSize;

uniform float divGridAlphaFrac;
uniform float divGridScale;
uniform vec2 divGridClosestPoint;
uniform float divGridPointSize;

float get_color_val_square_point(float2 fCoord, float gScale, float gPointSize, float2 gClosestPoint) {
    float2 modCoord = gScale > 100000.0 ? (fCoord - gClosestPoint + float2(0.5 * gScale)) : mod(fCoord - gClosestPoint + float2(0.5 * gScale), float2(gScale));

    float2 difToPointCenter = abs(modCoord - float2(0.5 * gScale));
    float m1 = 1.0 - smoothstep(gPointSize, gPointSize + 1.0, difToPointCenter.x);
    float m2 = 1.0 - smoothstep(gPointSize, gPointSize + 1.0, difToPointCenter.y);
    return m1 * m2;
}

vec4 main(float2 fragCoord) {
    float mainGridColor = get_color_val_square_point(fragCoord, mainGridScale, mainGridPointSize, mainGridClosestPoint);
    float divGridColor = get_color_val_square_point(fragCoord, divGridScale, divGridPointSize, divGridClosestPoint) * divGridAlphaFrac;
    float fin = max(mainGridColor, divGridColor);

    vec4 pointColorUnmultiplied = vec4(gridColor.rgb, 1.0);

  	return pointColorUnmultiplied * gridColor.a * fin;
})V";

const char* squareLinesShaderCode = R"V(
uniform vec4 gridColor;
uniform float mainGridScale;
uniform vec2 mainGridClosestPoint;
uniform float mainGridPointSize;

uniform float divGridAlphaFrac;
uniform float divGridScale;
uniform vec2 divGridClosestPoint;
uniform float divGridPointSize;

float get_color_val_square_lines(float2 fCoord, float gScale, float gPointSize, float2 gClosestPoint) {
    float2 modCoord = gScale > 100000.0 ? (fCoord - gClosestPoint + float2(0.5 * gScale)) : mod(fCoord - gClosestPoint + float2(0.5 * gScale), float2(gScale));
    float2 difToPointCenter = abs(modCoord - float2(0.5 * gScale));
    float m1 = 1.0 - smoothstep(gPointSize, gPointSize + 1.0, difToPointCenter.x);
    float m2 = 1.0 - smoothstep(gPointSize, gPointSize + 1.0, difToPointCenter.y);
    return max(m1, m2);
}

vec4 main(float2 fragCoord) {
    float mainGridColor = get_color_val_square_lines(fragCoord, mainGridScale, mainGridPointSize, mainGridClosestPoint);
    float divGridColor = get_color_val_square_lines(fragCoord, divGridScale, divGridPointSize, divGridClosestPoint) * divGridAlphaFrac;
    float fin = max(mainGridColor, divGridColor);

    vec4 pointColorUnmultiplied = vec4(gridColor.rgb, 1.0);

  	return pointColorUnmultiplied * gridColor.a * fin;
})V";

const char* horizontalLinesShaderCode = R"V(
uniform vec4 gridColor;
uniform float mainGridScale;
uniform vec2 mainGridClosestPoint;
uniform float mainGridPointSize;

uniform float divGridAlphaFrac;
uniform float divGridScale;
uniform vec2 divGridClosestPoint;
uniform float divGridPointSize;

float get_color_val_horizontal_lines(float2 fCoord, float gScale, float gPointSize, float2 gClosestPoint) {
    float2 modCoord = gScale > 100000.0 ? (fCoord - gClosestPoint + float2(0.5 * gScale)) : mod(fCoord - gClosestPoint + float2(0.5 * gScale), float2(gScale));
    float2 difToPointCenter = abs(modCoord - float2(0.5 * gScale));
    return 1.0 - smoothstep(gPointSize, gPointSize + 1.0, difToPointCenter.y);
}
vec4 main(float2 fragCoord) {
    float mainGridColor = get_color_val_horizontal_lines(fragCoord, mainGridScale, mainGridPointSize, mainGridClosestPoint);
    float divGridColor = get_color_val_horizontal_lines(fragCoord, divGridScale, divGridPointSize, divGridClosestPoint) * divGridAlphaFrac;
    float fin = max(mainGridColor, divGridColor);

    vec4 pointColorUnmultiplied = vec4(gridColor.rgb, 1.0);

  	return pointColorUnmultiplied * gridColor.a * fin;
})V";

sk_sp<SkRuntimeEffect> WorldGrid::compile_effect_shader_init(const char* shaderName, const char* shaderCode) {
    auto [effect, err] = SkRuntimeEffect::MakeForShader(SkString(shaderCode));
    if(!err.isEmpty()) {
        std::cout << "[GridManager::compile_effect_shader_init] " << shaderName << " shader construction error\n";
        std::cout << err.c_str() << std::endl;
        throw std::runtime_error("Shader Compile Failure");
    }
    return effect;
}

sk_sp<SkShader> WorldGrid::get_shader(GridType gType, const ShaderData& shaderData) {
    sk_sp<SkRuntimeEffect> runtimeEffect;
    switch(gType) {
        case GridType::CIRCLE_POINTS:
            runtimeEffect = circlePointEffect;
            break;
        case GridType::SQUARE_POINTS:
            runtimeEffect = squarePointEffect;
            break;
        case GridType::SQUARE_LINES:
            runtimeEffect = squareLinesEffect;
            break;
        case GridType::HORIZONTAL_LINES:
            runtimeEffect = horizontalLinesEffect;
            break;
    }
    SkRuntimeShaderBuilder builder(runtimeEffect);
    builder.uniform("gridColor") = shaderData.gridColor;
    builder.uniform("mainGridScale") = std::min(shaderData.mainGridScale, 1000000.0f); // Limit placed so that we dont have infinity in the shader
    builder.uniform("mainGridClosestPoint").set(shaderData.mainGridClosestPoint.data(), 2);

    builder.uniform("divGridAlphaFrac") = shaderData.divGridAlphaFrac;
    builder.uniform("divGridScale") = std::min(shaderData.divGridScale, 1000000.0f);
    builder.uniform("divGridClosestPoint").set(shaderData.divGridClosestPoint.data(), 2);
    switch(gType) {
        case GridType::CIRCLE_POINTS:
        case GridType::SQUARE_POINTS:
            builder.uniform("mainGridPointSize") = shaderData.mainGridPointSize;
            builder.uniform("divGridPointSize") = shaderData.divGridPointSize;
            break;
        case GridType::SQUARE_LINES:
        case GridType::HORIZONTAL_LINES:
            builder.uniform("mainGridPointSize") = 0.5f;
            builder.uniform("divGridPointSize") = 0.5f;
            break;
    }
    sk_sp<SkShader> s = builder.makeShader();
    return s;
}

Vector2f WorldGrid::get_closest_grid_point(const WorldVec& gridOffset, const WorldScalar& gridSize, const DrawData& drawData) {
    WorldVec fracCamPosOnGrid;
    fracCamPosOnGrid.x() = (drawData.cam.c.pos.x() - gridOffset.x()) % gridSize;
    fracCamPosOnGrid.y() = (drawData.cam.c.pos.y() - gridOffset.y()) % gridSize;

    WorldVec closestGridPoint;
    closestGridPoint.x() = (fracCamPosOnGrid.x() < gridSize / WorldScalar(2)) ? WorldScalar(0) : gridSize;
    closestGridPoint.y() = (fracCamPosOnGrid.y() < gridSize / WorldScalar(2)) ? WorldScalar(0) : gridSize;

    Vector2f closestGridPointScreenPos;
    closestGridPointScreenPos.x() = static_cast<float>((closestGridPoint.x() - fracCamPosOnGrid.x()) / drawData.cam.c.inverseScale);
    closestGridPointScreenPos.y() = static_cast<float>((closestGridPoint.y() - fracCamPosOnGrid.y()) / drawData.cam.c.inverseScale);

    return closestGridPointScreenPos;
}

void WorldGrid::set_remove_divisions_outwards(bool v) {
    removeDivisionsOutwards = v;
    if(removeDivisionsOutwards)
        subdivisions = std::max<uint32_t>(subdivisions, 2);
}

void WorldGrid::set_subdivisions(uint32_t v) {
    subdivisions = v;
    if(removeDivisionsOutwards)
        subdivisions = std::max<uint32_t>(subdivisions, 2);
}

std::string WorldGrid::get_display_name() {
    return name.empty() ? "New Grid" : name;
}

void WorldGrid::scale_up(const WorldScalar& scaleUpAmount) {
    size *= scaleUpAmount;
    offset *= scaleUpAmount;
}

void WorldGrid::draw(GridManager& gMan, SkCanvas* canvas, const DrawData& drawData) {
    coordinatesAxisOnBounds = false;
    coordinatesWillBeDrawn = false;

    if(!visible || (bounds.has_value() && !SCollision::collide(drawData.cam.viewingAreaGenerousCollider, bounds.value())))
        return;

    if(!circlePointEffect) {
        circlePointEffect = compile_effect_shader_init("Circle Point", circlePointShaderCode);
        squarePointEffect = compile_effect_shader_init("Square Point", squarePointShaderCode);
        squareLinesEffect = compile_effect_shader_init("Square Lines", squareLinesShaderCode);
        horizontalLinesEffect = compile_effect_shader_init("Horizontal Lines", horizontalLinesShaderCode);
    }

    WorldScalar sizeDetermineSubdivisionsToRemove = (drawData.cam.c.inverseScale / size) * WorldScalar(GRID_UNIT_PIXEL_SIZE);

    SkPaint linePaint;

    WorldScalar divSize;
    WorldScalar gridCoordDivSize;

    float gridPointSizeDefault = 4.0f;

    if(sizeDetermineSubdivisionsToRemove > WorldScalar(1)) {
        double divLogDouble, divLogFraction;
        divLogFraction = std::modf(static_cast<double>(FixedPoint::log(sizeDetermineSubdivisionsToRemove, WorldScalar(subdivisions == 1 ? 2 : subdivisions))), &divLogDouble);
        uint64_t divLog = divLogDouble;
        if(divLog >= 1 && !removeDivisionsOutwards)
            return;

        WorldScalar logMultiplier = FixedPoint::exp_int_accurate<WorldScalar>(divLog, subdivisions);

        WorldScalar subDivSize = size * logMultiplier;
        divSize = subDivSize * WorldScalar(subdivisions);
        WorldScalar subDivWorldSize = subDivSize / drawData.cam.c.inverseScale;
        WorldScalar divWorldSize = divSize / drawData.cam.c.inverseScale;

        float floatDivWorldSize = static_cast<float>(divWorldSize);
        float floatSubDivWorldSize = static_cast<float>(subDivWorldSize);

        Vector2f closestGridPointScreenPos = get_closest_grid_point(offset, divSize, drawData);
        Vector2f subClosestGridPointScreenPos = get_closest_grid_point(offset, subDivSize, drawData);

        SkColor4f pointColor = convert_vec4<SkColor4f>(color);

        float finalDivMultiplierValue = 1.0 - divLogFraction;
        float finalDivMultiplierSqrt = std::sqrt(finalDivMultiplierValue);
        float finalDivMultiplierSqrd = std::pow(finalDivMultiplierValue, 2);

        if(!removeDivisionsOutwards)
            pointColor.fA *= finalDivMultiplierSqrt;
        linePaint.setShader(get_shader(gridType, {
            .gridColor = pointColor,
            .mainGridScale = floatDivWorldSize,
            .mainGridClosestPoint = closestGridPointScreenPos, 
            .mainGridPointSize = gridPointSizeDefault * finalDivMultiplierSqrt,

            .divGridAlphaFrac = 0.3f,
            .divGridScale = floatSubDivWorldSize,
            .divGridClosestPoint = subClosestGridPointScreenPos,
            .divGridPointSize = gridPointSizeDefault * finalDivMultiplierSqrd
        }));

        gridCoordDivSize = logMultiplier * WorldScalar(subdivisions);
    }
    else {
        WorldScalar subDivSize = size;
        divSize = subDivSize * WorldScalar(subdivisions);
        WorldScalar subDivWorldSize = subDivSize / drawData.cam.c.inverseScale;
        WorldScalar divWorldSize = divSize / drawData.cam.c.inverseScale;

        float floatDivWorldSize = static_cast<float>(divWorldSize);
        float floatSubDivWorldSize = static_cast<float>(subDivWorldSize);

        Vector2f closestGridPointScreenPos = get_closest_grid_point(offset, divSize, drawData);
        Vector2f subClosestGridPointScreenPos = get_closest_grid_point(offset, subDivSize, drawData);

        SkColor4f pointColor = convert_vec4<SkColor4f>(color);
        float gridPointSize = std::clamp(floatDivWorldSize / 10.0f, 0.0f, 5.0f);

        linePaint.setShader(get_shader(gridType, {
            .gridColor = pointColor,
            .mainGridScale = floatDivWorldSize,
            .mainGridClosestPoint = closestGridPointScreenPos, 
            .mainGridPointSize = gridPointSizeDefault,

            .divGridAlphaFrac = 0.3f,
            .divGridScale = floatSubDivWorldSize,
            .divGridClosestPoint = subClosestGridPointScreenPos,
            .divGridPointSize = gridPointSize
        }));

        gridCoordDivSize = WorldScalar(subdivisions);
    }

    canvas->save();
    if(bounds.has_value()) {
        const auto& b = bounds.value();
        Vector2f b1 = drawData.cam.c.to_space(b.min);
        Vector2f b2 = drawData.cam.c.to_space(b.top_right());
        Vector2f b3 = drawData.cam.c.to_space(b.max);
        Vector2f b4 = drawData.cam.c.to_space(b.bottom_left());
        SkPath clipPath;
        clipPath.moveTo(b1.x(), b1.y());
        clipPath.lineTo(b2.x(), b2.y());
        clipPath.lineTo(b3.x(), b3.y());
        clipPath.lineTo(b4.x(), b4.y());
        clipPath.close();
        canvas->clipPath(clipPath);
    }
    canvas->rotate(-drawData.cam.c.rotation * 180.0 / std::numbers::pi);
    canvas->drawPaint(linePaint);
    canvas->restore();
    if(showCoordinates && drawData.cam.c.rotation == 0.0 && !drawData.main->takingScreenshot) {
        coordinatesWillBeDrawn = true;
        WorldVec worldWindowEndPos = drawData.cam.c.pos + drawData.cam.c.dir_from_space(drawData.cam.viewingArea);
        coordinatesAxisOnBounds = bounds.has_value() && !bounds.value().fully_contains_aabb(SCollision::AABB<WorldScalar>(drawData.cam.c.pos, worldWindowEndPos));
        coordinatesDivWorldSize = divSize;
        coordinatesGridCoordDivSize = gridCoordDivSize;
    }
}

void WorldGrid::draw_coordinates(SkCanvas* canvas, const DrawData& drawData, Vector2f& axisOffset) {
    auto& divWorldSize = coordinatesDivWorldSize;
    auto& gridCoordDivSize = coordinatesGridCoordDivSize;

    WorldVec worldWindowEndPos = drawData.cam.c.pos + drawData.cam.c.dir_from_space(drawData.cam.viewingArea);

    const WorldVec& worldWindowTopLeftClipPos = coordinatesAxisOnBounds ? bounds.value().min : drawData.cam.c.pos;
    SCollision::AABB<float> boundsCamSpace;
    float coordFontSize = drawData.main->toolbar.final_gui_scale() * drawData.main->toolbar.io->fontSize;

    if(coordinatesAxisOnBounds)
        boundsCamSpace = SCollision::AABB<float>(drawData.cam.c.to_space(bounds.value().min), drawData.cam.c.to_space(bounds.value().max));
    else {
        float toolbarXLength = drawData.main->drawGui ? drawData.main->toolbar.final_gui_scale() * (50.0f + drawData.main->toolbar.io->theme->padding1 * 3.0f) : 0.0f;
        boundsCamSpace = SCollision::AABB<float>({toolbarXLength + axisOffset.x(), 0.0f}, {drawData.cam.viewingArea.x(), drawData.cam.viewingArea.y() - axisOffset.y()});
        if(boundsCamSpace.width() < 100.0f || boundsCamSpace.height() < 100.0f)
            return;
    }

    if(coordinatesAxisOnBounds) {
        float fontSizeBoundsMultiplier = std::min(std::min(boundsCamSpace.width() / drawData.cam.viewingArea.x(), boundsCamSpace.height() / drawData.cam.viewingArea.y()) * 4.0f, 1.0f);
        coordFontSize *= fontSizeBoundsMultiplier;
        if(coordFontSize <= 3.0f)
            return;
    }

    SkFont f = drawData.main->toolbar.io->get_font(coordFontSize);
    SkFontMetrics metrics;
    f.getMetrics(&metrics);
    float fontHeight = (-metrics.fAscent + metrics.fDescent);

    WorldVec worldWindowBeginPos;
    WorldVec coordMultiplier;

    auto calculateCoordMultipliers = [&]() {
        coordMultiplier.x() = FixedPoint::trunc((drawData.cam.c.pos.x() - offset.x()) / divWorldSize);
        worldWindowBeginPos.x() = coordMultiplier.x() * divWorldSize + offset.x();
        coordMultiplier.y() = -FixedPoint::trunc((offset.y() - drawData.cam.c.pos.y() + divWorldSize) / divWorldSize);
        worldWindowBeginPos.y() = coordMultiplier.y() * divWorldSize + offset.y();
    };
    calculateCoordMultipliers();

    struct NumberTextData {
        std::string mainText;
        float mainPos; // X or Y pos depending on axis
        float mainLength; // X length

        std::string exponentText;
        float exponentPos;
        float exponentLength;

        float totalLength;
    };

    std::vector<NumberTextData> xAxisLabels, yAxisLabels;
    bool exponentExistsInXAxis;

    float yAxisMaxXLength = 0.0f;

    for(int maxRepeats = 0; maxRepeats < 10; maxRepeats++) {
        xAxisLabels.clear();
        exponentExistsInXAxis = false;

        WorldScalar xGridCoord = coordMultiplier.x() * gridCoordDivSize;

        for(WorldScalar xWorldCoord = worldWindowBeginPos.x(); xWorldCoord < worldWindowEndPos.x(); xWorldCoord += divWorldSize) {
            NumberTextData label;

            Vector2f gridPointScreenPos = drawData.cam.c.to_space(WorldVec{xWorldCoord, worldWindowBeginPos.y()});

            label.mainText = xGridCoord.display_int_str(4, false, &label.exponentText);
            if(!label.exponentText.empty()) {
                label.mainText += "×10";
                exponentExistsInXAxis = true;
            }
            label.mainLength = f.measureText(label.mainText.c_str(), label.mainText.length(), SkTextEncoding::kUTF8, nullptr);
            label.exponentLength = f.measureText(label.exponentText.c_str(), label.exponentText.length(), SkTextEncoding::kUTF8, nullptr);
            label.totalLength = label.mainLength + label.exponentLength;
            label.mainPos = gridPointScreenPos.x() - (label.mainLength * 0.5f + label.exponentLength * 0.5f);
            label.exponentPos = label.mainPos + label.mainLength;

            xGridCoord += gridCoordDivSize;

            if(xWorldCoord > worldWindowTopLeftClipPos.x())
                xAxisLabels.emplace_back(label);
        }

        bool labelsTooClose = false;
        for(size_t i = 1; i < xAxisLabels.size(); i++) {
            if(xAxisLabels[i - 1].mainPos + xAxisLabels[i - 1].totalLength >= xAxisLabels[i].mainPos - 10.0f * drawData.main->toolbar.final_gui_scale()) {
                divWorldSize *= WorldScalar(2);
                gridCoordDivSize *= WorldScalar(2);
                calculateCoordMultipliers();
                labelsTooClose = true;
                break;
            }
        }
        if(!labelsTooClose)
            break;
    }

    float xAxisYPosRect = boundsCamSpace.max.y() - (exponentExistsInXAxis ? fontHeight * 1.25f : fontHeight);

    {
        WorldScalar yGridCoord = FixedPoint::trunc((offset.y() - drawData.cam.c.pos.y()) / divWorldSize) * gridCoordDivSize + gridCoordDivSize;

        for(WorldScalar yWorldCoord = worldWindowBeginPos.y(); yWorldCoord < worldWindowEndPos.y(); yWorldCoord += divWorldSize) {
            NumberTextData label;

            Vector2f gridPointScreenPos = drawData.cam.c.to_space(WorldVec{worldWindowBeginPos.x(), yWorldCoord});

            label.mainText = yGridCoord.display_int_str(4, false, &label.exponentText);
            if(!label.exponentText.empty())
                label.mainText += "×10";
            label.mainLength = f.measureText(label.mainText.c_str(), label.mainText.length(), SkTextEncoding::kUTF8, nullptr);
            label.exponentLength = f.measureText(label.exponentText.c_str(), label.exponentText.length(), SkTextEncoding::kUTF8, nullptr);
            label.totalLength = label.mainLength + label.exponentLength;
            label.mainPos = gridPointScreenPos.y() + metrics.fDescent;
            label.exponentPos = label.mainPos - fontHeight * 0.25f;

            yGridCoord -= gridCoordDivSize;

            if(yWorldCoord > worldWindowTopLeftClipPos.y() && label.mainPos < xAxisYPosRect && label.mainPos - fontHeight * 1.25f > boundsCamSpace.min.y()) {
                yAxisMaxXLength = std::max(yAxisMaxXLength, label.totalLength);
                yAxisLabels.emplace_back(label);
            }
        }
    }

    std::erase_if(xAxisLabels, [&](auto& l) {
        return !(l.mainPos > boundsCamSpace.min.x() + yAxisMaxXLength && l.mainPos + l.totalLength < boundsCamSpace.max.x());
    });

    if(!coordinatesAxisOnBounds) {
        axisOffset.x() += yAxisMaxXLength;
        if(!xAxisLabels.empty())
            axisOffset.y() += (exponentExistsInXAxis ? fontHeight * 1.25f : fontHeight);
    }

    SkPaint labelColor(SkColor4f{color.x(), color.y(), color.z(), 1.0f});

    float xAxisYPosLabels = boundsCamSpace.max.y() - metrics.fDescent;

    if(coordinatesAxisOnBounds) {
        canvas->save();
        canvas->clipRect(SkRect::MakeLTRB(boundsCamSpace.min.x(), boundsCamSpace.min.y(), boundsCamSpace.max.x(), boundsCamSpace.max.y()));
    }

    canvas->drawRect(SkRect::MakeXYWH(boundsCamSpace.min.x(), boundsCamSpace.min.y(), yAxisMaxXLength, boundsCamSpace.max.y() - boundsCamSpace.min.y()), SkPaint{color_mul_alpha(drawData.main->toolbar.io->theme->backColor1, 0.9f)});
    if(!xAxisLabels.empty())
        canvas->drawRect(SkRect::MakeLTRB(boundsCamSpace.min.x() + yAxisMaxXLength, xAxisYPosRect, boundsCamSpace.max.x(), boundsCamSpace.max.y()), SkPaint{color_mul_alpha(drawData.main->toolbar.io->theme->backColor1, 0.9f)});

    for(auto& l : xAxisLabels) {
        canvas->drawSimpleText(l.mainText.c_str(), l.mainText.length(), SkTextEncoding::kUTF8, l.mainPos, xAxisYPosLabels, f, labelColor);
        canvas->drawSimpleText(l.exponentText.c_str(), l.exponentText.length(), SkTextEncoding::kUTF8, l.exponentPos, xAxisYPosLabels - fontHeight * 0.25f, f, labelColor);
    }

    for(auto& l : yAxisLabels) {
        float mainTextXPos = boundsCamSpace.min.x() + yAxisMaxXLength * 0.5f - l.totalLength * 0.5f;
        canvas->drawSimpleText(l.mainText.c_str(), l.mainText.length(), SkTextEncoding::kUTF8, mainTextXPos, l.mainPos, f, labelColor);
        canvas->drawSimpleText(l.exponentText.c_str(), l.exponentText.length(), SkTextEncoding::kUTF8, mainTextXPos + l.mainLength, l.exponentPos, f, labelColor);
    }

    if(coordinatesAxisOnBounds)
        canvas->restore();
}
