#include "StringHelpers.hpp"
#include <fstream>
#include <algorithm>
#include <regex>

std::vector<std::string> split_string_by_token(std::string str, std::string token) {
    // slightly modified from: https://stackoverflow.com/a/46943631
    std::vector<std::string> toRet;
    while(!str.empty()) {
        size_t index = str.find(token);
        if(index != std::string::npos) {
            toRet.emplace_back(str.substr(0, index));
            str = str.substr(index + token.size());
            if(str.empty())
                toRet.emplace_back(str);
        }
        else {
            toRet.emplace_back(str);
            return toRet;
        }
    }
    return toRet;
}

uint8_t ascii_hex_char_to_number_no_checks(char asciiHex) {
    if(asciiHex <= '9')
        return static_cast<uint8_t>(asciiHex - '0');
    else
        return static_cast<uint8_t>(asciiHex - 'A');
}

std::string byte_vector_to_hex_str(const std::vector<uint8_t>& byteVec) {
    static constexpr char charList[] = "0123456789ABCDEF";

    std::string ret;
    ret.reserve(byteVec.size() * 2);

    for(uint8_t b : byteVec) {
        ret.push_back(charList[b >> 4]);
        ret.push_back(charList[b & 0x0F]);
    }

    return ret;
}

std::string read_file_to_string(const std::filesystem::path& filePath) {
    std::string toRet;
    // https://nullptr.org/cpp-read-file-into-string/
    std::ifstream file(filePath, std::ios::in | std::ios::binary | std::ios::ate);

    if(!file.is_open())
        throw std::runtime_error("[read_file_to_string] Could not open file");

    size_t resourcesize;

    auto tellgResult = file.tellg();

    if(tellgResult == -1)
        throw std::runtime_error("[read_file_to_string] tellg failed for file");

    resourcesize = static_cast<size_t>(tellgResult);

    file.seekg(0, std::ios_base::beg);
    toRet.resize(resourcesize);
    file.read(toRet.data(), resourcesize);
    file.close();

    return toRet;
}

bool is_valid_http_url(const std::string& str) {
    if(str.empty())
        return false;

    // Regex to check valid URL (from https://stackoverflow.com/a/3809435)
    std::regex pattern(R"rawstr(https?:\/\/(www\.)?[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b([-a-zA-Z0-9()@:%_\+.~#?&//=]*))rawstr");

    return std::regex_match(str, pattern);
}

std::string remove_carriage_returns_from_str(std::string s) {
    std::erase(s, '\r');
    return s;
}

std::string ensure_string_unique(const std::vector<std::string>& stringList, std::string str) {
    for(;;) {
        bool isUnique = true;
        for(const std::string& strToCheckAgainst : stringList) {
            if(strToCheckAgainst == str) {
                size_t leftParenthesisIndex = str.find_last_of('(');
                size_t rightParenthesisIndex = str.find_last_of(')');
                bool failToIncrement = true;
                if(leftParenthesisIndex != std::string::npos && rightParenthesisIndex != std::string::npos && leftParenthesisIndex < rightParenthesisIndex) {
                    std::string numStr = str.substr(leftParenthesisIndex + 1, rightParenthesisIndex - leftParenthesisIndex - 1);
                    bool isAllDigits = true;
                    for(char c : numStr) {
                        if(!isdigit(c)) {
                            isAllDigits = false;
                            break;
                        }
                    }
                    if(isAllDigits) {
                        try {
                            int s = std::stoi(numStr);
                            s++;
                            str = str.substr(0, leftParenthesisIndex);
                            str += "(" + std::to_string(s) + ")";
                            failToIncrement = false;
                        }
                        catch(...) {}
                    }
                }
                if(failToIncrement)
                    str += " (2)";
                isUnique = false;
                break;
            }
        }
        if(isUnique)
            break;
    }
    return str;
}
