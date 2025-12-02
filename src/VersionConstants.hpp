#pragma once
#include <Helpers/VersionNumber.hpp>
#include <unordered_map>

namespace VersionConstants {
    // Map OLDEST version that is compatible with the filetype with a specific header
    // This means that, to check for file version, you can do these two checks:
    // Check if older than a certain version: if(fileVersion < NEWEST VERSION YOU SHOULDNT RUN THE IF STATEMENT ON)
    // Check if newer/equal to a certain version: if(fileVersion >= OLDEST VERSION YOU SHOULD RUN THE IF STATEMENT ON)
    VersionNumber header_to_version_number(const std::string& header); 

    constexpr int SAVEFILE_HEADER_LEN = 12; // DO NOT CHANGE THIS HEADER LENGTH
    const std::string CURRENT_SAVEFILE_HEADER = "INFPNT000004"; // Change whenever the save file is incompatible with the previous version
    const std::string CURRENT_VERSION_STRING = "0.3.1";
    constexpr VersionNumber CURRENT_VERSION_NUMBER(0, 3, 1);
}
