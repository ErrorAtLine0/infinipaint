#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"

int main()
{
    sk_sp<SkSurface> surface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul({1280, 720}));

    SkCanvas* canvas = surface->getCanvas();

    SkPaint p;
    p.setColor(SK_ColorRED);
    p.setAntiAlias(true);
    p.setStyle(SkPaint::kStroke_Style);
    p.setStrokeWidth(10);

    canvas->drawLine(20, 20, 100, 100, p);
}
