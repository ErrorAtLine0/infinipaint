#include "../src/BrushPressureConfig.hpp"
#include "../src/BrushSampleWidths.hpp"
#include "../src/PenStabilizer.hpp"
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
        for (auto mode : {Response::Original,Response::Preserve,Response::Peak}) {
            Config c; c.pressureResponse=mode;
            bool correction=true;
            c.migrateCorrection(correction);
            require(correction==(mode!=Response::Original),"old inactive filter migration");
            correction=true;
            for(auto next : {Response::Original,Response::Preserve,Response::Peak}) {
                c.pressureResponse=next;
                c.migrateCorrection(correction);
                require(correction && c.samplePath(correction),"pressure mode disabled correction");
            }
            auto restored=json(c).get<Config>();
            require(restored.correctionIndependent,"migration marker lost");
        }
        for(float factor : {0.0f,.707f,1.0f}) {
            SampleWidths smooth;
            smooth.reset(false,1,true,factor);
            smooth.append(1); smooth.append(1); smooth.append(10); smooth.append(1);
            require(smooth.output(10,3)==10,"peak lost in smoothed widths");
            require(smooth.output(1,0)==(factor==1 ? 10 : factor==0 ? 1 : 10*factor*factor*factor),"backward width propagation");
            require(smooth.output(1,4)==std::max(1.0f,10*factor),"forward width propagation");
            smooth.reset(false,.25f,true,factor);
            require(smooth.output(.25f,0)==.25f,"smoothed state leaked");
        }
        for (bool enabled : {false,true}) for (bool peak : {false,true}) {
            PenInput::Stabilizer path; path.reset({enabled});
            SampleWidths widths; widths.reset(peak,1);
            std::vector<float> rendered;
            for (unsigned i=0;i<120;++i) {
                // Includes stationary pressure and a peak after a frozen prefix.
                const float w=i==70 ? 10.0f : 1.0f;
                require(path.append({{i<75 ? double(i) : 74.0, double(i%3)},i*.01,w}),"append");
                const bool grew=widths.append(w);
                const auto start=grew ? 0 : path.changedBegin();
                rendered.resize(path.samples().size());
                for (auto k=start;k<rendered.size();++k) rendered[k]=widths.output(path.samples()[k].width);
                for (unsigned k=0;k<=i;++k) {
                    const float expected=peak ? (i>=70 ? 10.0f : 1.0f) : (k==70 ? 10.0f : 1.0f);
                    require(rendered[k]==expected,"per-point/peak width semantics");
                    require(path.samples()[k].width==(k==70 ? 10.0f : 1.0f),"source widths changed");
                }
            }
            const auto before=rendered;
            path.finish();
            require(rendered==before,"finish changed widths");
            widths.reset(peak,.25f);
            require(widths.output(.25f)==.25f,"peak leaked between strokes");
        }
        // All three policies use identical corrected positions. Attribute changes
        // must neither bypass the filter nor mutate its immutable input.
        for(bool enabled : {false,true}) {
            PenInput::Stabilizer path; path.reset({enabled});
            SampleWidths preserve, peak, smooth;
            preserve.reset(false,1); peak.reset(true,1); smooth.reset(false,1,true,.707f);
            require(path.append({{0,0},0,1}),"first report");
            for(unsigned i=1;i<180;++i) {
                const float width=i==100 ? 10.0f : 1.0f;
                require(path.append({{double(i),std::sin(i*.5)},i*.01,width}),"mode append");
                const auto positions=path.positions();
                preserve.append(width); peak.append(width); smooth.append(width);
                for(size_t k=0;k<path.samples().size();++k) {
                    const auto raw=path.samples()[k].width;
                    require(preserve.output(raw,k)==raw,"preserve changed width");
                    require(peak.output(raw,k)==(i>=100 ? 10 : 1),"peak did not propagate");
                    require(std::isfinite(smooth.output(raw,k)),"invalid smoothed width");
                    require(path.positions()[k].x==positions[k].x && path.positions()[k].y==positions[k].y,"pressure changed positions");
                    require(raw==(k==100 ? 10 : 1),"pressure changed source");
                }
            }
        }
        std::cout << "Pressure migration and independent width policies passed\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
