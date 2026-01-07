#include "GUIManagerID.hpp"

namespace GUIStuff {

GUIManagerID::GUIManagerID(intptr_t numID):
    idIsStr(false),
    idNum(numID)
{}

GUIManagerID::GUIManagerID(const char* strID):
    idIsStr(true),
    idNum(reinterpret_cast<intptr_t>(strID))
{}

bool GUIManagerID::operator==(const GUIManagerID& other) const {
    return idIsStr == other.idIsStr && idNum == other.idNum;
}

}
