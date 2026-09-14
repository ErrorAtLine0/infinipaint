#include "../src/BrushPressureConfig.hpp"
#include "../src/BrushSampleWidths.hpp"
#include <iostream>
#include <stdexcept>

void require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
int main() {
    try {
        using namespace BrushPressure;
        using nlohmann::json;
        require(json::object().get<Config>().pressureResponse == Response::Original, "old default");
        require(json{{"preservePenPressure",true}}.get<Config>().pressureResponse == Response::Preserve, "old opt-in migration");
        require(json{{"preservePenPressure",false}}.get<Config>().pressureResponse == Response::Original, "old off migration");
        require(json{{"pressureResponse","future"},{"preservePenPressure",true}}.get<Config>().pressureResponse == Response::Original, "unknown mode fallback");
        require(json{{"pressureResponse",42}}.get<Config>().pressureResponse == Response::Original, "invalid mode fallback");
        for (auto mode : {Response::Original,Response::Preserve,Response::Peak}) {
            Config c; c.pressureResponse=mode; c.hasRoundCaps=false; c.relativeWidth=37;
            const auto restored=json(c).get<Config>();
            require(restored.pressureResponse==mode && !restored.hasRoundCaps && restored.relativeWidth==37, "configuration roundtrip");
            require(restored.samplePath()==(mode!=Response::Original), "engine mapping");
        }
        for (bool peak : {false,true}) {
            SampleWidths widths; widths.reset(peak,1);
            std::vector<float> rendered{1};
            for (unsigned i=1;i<120;++i) {
                const float width=i==70 ? 10.0f : 1.0f;
                const bool changed=widths.append(width);
                rendered.push_back(widths.output(width));
                if(changed) for(auto& w:rendered) w=widths.output(w);
                for(unsigned k=0;k<=i;++k)
                    require(rendered[k]==(peak ? (i>=70 ? 10.0f : 1.0f) : (k==70 ? 10.0f : 1.0f)), "pressure policy");
            }
            widths.reset(peak,.25f);
            require(widths.output(.25f)==.25f,"state leaked across strokes");
        }
        std::cout << "Pressure configuration and width policy checks passed\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
