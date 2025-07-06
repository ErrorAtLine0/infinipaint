#pragma once
#include <iostream>
#include <sstream>
#include <memory>
#include <vector>

// https://tuttlem.github.io/2014/08/18/getting-istream-to-work-off-a-byte-array.html
class ByteMemBuf : public std::basic_streambuf<char> {
    public:
        ByteMemBuf(char* p, size_t l);
};
class ByteMemStream : public std::istream {
    public:
        ByteMemStream(char* p, size_t l);
    private:
        ByteMemBuf buffer;
};

std::vector<std::shared_ptr<std::stringstream>> fragment_message(std::string_view v, uint64_t stride);
