#pragma once
#include <filesystem>

std::filesystem::path force_extension_on_path(std::filesystem::path p, const std::string& extensions);
