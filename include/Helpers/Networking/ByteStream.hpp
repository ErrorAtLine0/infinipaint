/*  
 * InfiniPaint
 * Copyright (C) 2025-2026 Yousef Khadadeh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

std::vector<std::shared_ptr<std::stringstream>> fragment_message(std::string_view uncompressedView, size_t stride);
void decode_fragmented_message(cereal::PortableBinaryInputArchive& inArchive, PartialFragmentMessage& spfm, size_t fragmentMessageStride, std::function<void(cereal::PortableBinaryInputArchive& completeArchive)> runAfterDecoded);
