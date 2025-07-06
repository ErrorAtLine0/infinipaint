#include "FileHelpers.hpp"
#include <Helpers/StringHelpers.hpp>
#include <algorithm>

std::filesystem::path force_extension_on_path(std::filesystem::path p, const std::string& extensions) {
    std::vector<std::string> extensionList = split_string_by_token(extensions, ";");
    for(std::string& s : extensionList) {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        s.insert(0, ".");
    }
    

    std::string fExtension;
    if(p.has_extension()) {
        fExtension = p.extension().string();
        std::transform(fExtension.begin(), fExtension.end(), fExtension.begin(), ::tolower);
    }

    if(std::ranges::contains(extensionList, fExtension))
        return p;

    p += extensionList[0];
    return p;
}
