#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

std::vector<std::string> split_string_by_token(std::string str, std::string token);

// Hex character must be capitalized if it is a letter
uint8_t ascii_hex_char_to_number_no_checks(char asciiHex);

std::string byte_vector_to_hex_str(const std::vector<uint8_t>& byteVec);

std::string read_file_to_string(const std::filesystem::path& filePath);
