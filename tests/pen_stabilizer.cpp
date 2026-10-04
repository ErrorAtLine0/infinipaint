#include "../src/PenStabilizer.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdexcept>
#ifdef PENTRACE_REFERENCE
#include "core.hpp"
#endif

using namespace PenInput;
void require(bool ok, const char* why) { if (!ok) throw std::runtime_error(why); }
bool near(Point a, Point b, double tolerance=1e-7) { return (a-b).length() < tolerance; }

#ifdef PENTRACE_REFERENCE
// Compare every live prefix with the original diagnostic's batch implementation,
// not a second copy of our streaming algorithm. No private recording is uploaded.
void reference_test(int hz, int kind) {
    Stabilizer s;
    s.reset();
    pt::Stroke stroke;
    stroke.kind=pt::Kind::Pen;
    for (int i=0; i<hz*2; ++i) {
        const double t=static_cast<double>(i)/hz;
        Point p{80*t, 80*t+std::sin(t*60)};
        if (kind==1) p={25*std::cos(t*8),25*std::sin(t*8)};
        if (kind==2) p=i<hz ? Point{80*t,0} : Point{80,80*(t-1)};
        if (kind==3) p={i/5.0, static_cast<double>((i/5)%2)};
        pt::Sample sample;
        sample.p={p.x,p.y}; sample.time=t; sample.contact=true;
        sample.kind=pt::Kind::Pen; sample.clock=pt::Clock::Qpc;
        // Inject stationary samples, equal time, and a gap.
        if(kind==3 && i>0 && i%7==0) sample.p=stroke.points.back().p;
        if(kind==3 && i>0 && i%47==0) sample.time=stroke.points.back().time;
        if(kind==3 && i>hz) sample.time+=.080;
        stroke.points.push_back(sample);
        require(s.append({{sample.p.x,sample.p.y},sample.time,.3f}),"append");
        const auto expected=pt::localFilter(stroke,12,4,.120);
        require(expected.size()==s.positions().size(),"point count");
        for(std::size_t k=0;k<expected.size();++k)
            require(near(s.positions()[k],{expected[k].x,expected[k].y}),"PenTraceLab prefix parity");
    }
}
#endif

int main() {
    try {
#ifdef PENTRACE_REFERENCE
        for(int hz:{60,120,240,672}) for(int kind=0;kind<4;++kind) reference_test(hz,kind);
#endif
        Stabilizer s,rotated,off;
        s.reset(); rotated.reset(); off.reset({false});
        double rawEnergy=0,filteredEnergy=0;
        std::vector<Point> frozen;
        for(int i=0;i<1600;++i) {
            const double t=i/400.0;
            const Point p{80*t,80*t+std::sin(t*60)};
            const auto before=s.positions();
            s.append({p,t,static_cast<float>(.1+t*.01)});
            rotated.append({{-p.y,p.x},t,.7f});
            off.append({p,t,.7f});
            for(std::size_t k=0;k<s.changedBegin();++k)
                require(near(s.positions()[k],before[k],1e-12),"frozen prefix changed");
            require(near(s.positions().back(),p,1e-12),"live endpoint moved");
            for(std::size_t k=0;k<s.positions().size();++k) {
                const auto q=s.positions()[k];
                require(q.finite(),"nonfinite output");
                require(near(rotated.positions()[k],{-q.y,q.x}),"rotation invariance");
                require((q-s.samples()[k].position).length()<=4.0000001,"cap exceeded");
                require(near(off.positions()[k],off.samples()[k].position,1e-12),"off not raw");
                require(s.samples()[k].width==static_cast<float>(.1+(k/400.0)*.01),"pressure rewritten");
            }
        }
        for(std::size_t i=80;i+80<s.positions().size();++i) {
            auto p=s.samples()[i].position, q=s.positions()[i];
            rawEnergy+=(p.y-p.x)*(p.y-p.x);
            filteredEnergy+=(q.y-q.x)*(q.y-q.x);
        }
        require(filteredEnergy<rawEnergy*.8,"synthetic regression, not hardware accuracy");
        auto previous=s.positions();
        require(!s.append({{std::numeric_limits<double>::quiet_NaN(),0},4,1}),"accepted NaN");
        require(s.positions().size()==previous.size(),"invalid sample changed size");
        s.append({{999,999},4.2,1});
        for(std::size_t i=0;i<previous.size();++i)
            require(near(s.positions()[i],previous[i],1e-12),"filtered across gap");
        s.append({{1000,999},4.2,.1f});
        require(near(s.positions().back(),{1000,999},1e-12),"equal-time report discarded");
        s.append({{1001,999},4.19,.2f});
        require(near(s.positions().back(),{1001,999},1e-12),"backward-time report discarded");
        s.reset();
        s.append({{2,3},0,.5f});
        require(s.positions().size()==1 && near(s.positions()[0],{2,3}),"dot/reset");
        for(int i=1;i<100;++i) s.append({{2,3},i*.001,.2f});
        for(auto p:s.positions()) require(near(p,{2,3}),"stationary drift");
        Settings bad{true,std::numeric_limits<double>::quiet_NaN(),100,-1};
        s.reset(bad);
        require(s.settings().radius==12 && s.settings().window==.2 && s.settings().cap==0,"settings bounds");
        std::cout<<"Local pen filter checks passed\n";
    } catch(const std::exception& e) {
        std::cerr<<e.what()<<'\n'; return EXIT_FAILURE;
    }
}
