#pragma once
#include <iostream>
#include <sstream>
#include <memory>
#include <vector>
#include <cereal/archives/portable_binary.hpp>

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

struct PartialFragmentMessage {
    std::string partialFragmentMessage;
    uint64_t partialFragmentMessageLoc = 0;
};

std::vector<std::shared_ptr<std::stringstream>> fragment_message(std::string_view uncompressedView, uint64_t stride);
void decode_fragmented_message(cereal::PortableBinaryInputArchive& inArchive, PartialFragmentMessage& spfm, size_t fragmentMessageStride, std::function<void(cereal::PortableBinaryInputArchive& completeArchive)> runAfterDecoded);
