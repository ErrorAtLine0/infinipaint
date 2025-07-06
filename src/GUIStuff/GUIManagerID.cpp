#include "GUIManagerID.hpp"

namespace GUIStuff {

GUIManagerID::GUIManagerID(int64_t numID):
    idIsStr(false),
    idNum(numID)
{}

GUIManagerID::GUIManagerID(const std::string& strID):
    idIsStr(true),
    idStr(strID)
{}

bool GUIManagerID::operator==(const GUIManagerID& other) const {
    return idIsStr == other.idIsStr && idStr == other.idStr && idNum == other.idNum;
}

}
