#pragma once
#include <string>
#include <optional>

struct VersionNumber {
    unsigned major;
    unsigned minor;
    unsigned patch;
};

std::optional<VersionNumber> version_str_to_version_numbers(const std::string& versionStr);
std::string version_numbers_to_version_str(const VersionNumber& versionNum);
bool operator>(const VersionNumber& a, const VersionNumber& b);
bool operator==(const VersionNumber& a, const VersionNumber& b);
