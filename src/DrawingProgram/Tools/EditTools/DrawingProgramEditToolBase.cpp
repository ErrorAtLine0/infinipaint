#include "DrawingProgramEditToolBase.hpp"
#include "../../../MainProgram.hpp"

DrawingProgramEditToolBase::DrawingProgramEditToolBase(DrawingProgram& initDrawP):
    drawP(initDrawP)
{}

bool DrawingProgramEditToolBase::right_click_popup_gui(const CanvasComponentContainer::ObjInfoSharedPtr& comp, Vector2f popupPos) {
    Toolbar& t = drawP.world.main.toolbar;
    t.paint_popup(popupPos);
    return true;
}

DrawingProgramEditToolBase::~DrawingProgramEditToolBase() {}
