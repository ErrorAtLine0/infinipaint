#include "VersionConstants.hpp"
#include <stdexcept>

namespace VersionConstants {

VersionNumber header_to_version_number(const std::string& header) {
    static std::unordered_map<std::string, VersionNumber> m;
    if(m.empty()) {
        m["INFPNT000001"] = VersionNumber(0, 0, 1);
        m["INFPNT000002"] = VersionNumber(0, 1, 0);
        m["INFPNT000003"] = VersionNumber(0, 2, 0);
        m["INFPNT000004"] = VersionNumber(0, 3, 0);
    }
    auto it = m.find(header);
    if(it == m.end())
        throw std::runtime_error("[VersionConstants::header_to_version_number] Header " + header + " is invalid");
    return it->second;
}

}
