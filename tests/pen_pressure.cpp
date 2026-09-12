#include "../src/PenPressure.hpp"
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

void require(bool ok) { if(!ok) throw std::runtime_error("Pressure mapping regression"); }
int main() {
    try {
        using PenInput::pressureFactor;
        require(pressureFactor(0, .2f, true)==0);
        require(pressureFactor(1, .2f, true)==1);
        require(std::abs(pressureFactor(.5f, .2f, true)-.6f)<1e-6f);
        require(pressureFactor(0, 0, false)==1);
        require(pressureFactor(-1, 0, true)==0);
        require(pressureFactor(2, 0, true)==1);
        require(pressureFactor(std::numeric_limits<float>::quiet_NaN(),0,true)==0);
        require(pressureFactor(.5f,std::numeric_limits<float>::quiet_NaN(),true)==.5f);
        std::array<float,5> widths{};
        const std::array<float,5> reports{.1f,.3f,1,.3f,.1f};
        for(std::size_t i=0;i<reports.size();++i) {
            const auto previous=widths;
            widths[i]=10*pressureFactor(reports[i],0,true);
            for(std::size_t j=0;j<i;++j) require(widths[j]==previous[j]);
        }
        require(widths.front()==1 && widths[2]==10 && widths.back()==1);
        std::cout<<"Per-report pressure mapping passed\n";
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
