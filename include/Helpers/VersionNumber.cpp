#include "VersionNumber.hpp"
#include "StringHelpers.hpp"

std::string version_numbers_to_version_str(const VersionNumber& versionNum) {
    return std::to_string(versionNum.major) + "." + std::to_string(versionNum.minor) + "." + std::to_string(versionNum.patch);
}

bool operator<=(const VersionNumber& a, const VersionNumber& b) {
    return !(a > b);
}

bool operator<(const VersionNumber& a, const VersionNumber& b) {
    return !(a >= b);
}

bool operator>=(const VersionNumber& a, const VersionNumber& b) {
    return (a > b) || (a == b);
}

bool operator>(const VersionNumber& a, const VersionNumber& b) {
    return (a.major > b.major) || (a.major == b.major && (a.minor > b.minor || (a.minor == b.minor && a.patch > b.patch)));
}

bool operator==(const VersionNumber& a, const VersionNumber& b) {
    return a.major == b.major && a.minor == b.minor && a.patch == b.patch;
}

bool operator!=(const VersionNumber& a, const VersionNumber& b) {
    return a.major != b.major || a.minor != b.minor || a.patch != b.patch;
}

std::optional<VersionNumber> version_str_to_version_numbers(const std::string& versionStr) {
    try {
        std::vector<std::string> splitStrings = split_string_by_token(versionStr, ".");
        if(splitStrings.size() != 3)
            return std::nullopt;
        VersionNumber toRet;
        toRet.major = std::stoul(splitStrings[0]);
        toRet.minor = std::stoul(splitStrings[1]);
        toRet.patch = std::stoul(splitStrings[2]);
        return toRet;
    }
    catch(...) {
    }
    return std::nullopt;
}
