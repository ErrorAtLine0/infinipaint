#include "ByteStream.hpp"
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/string.hpp>
#include "NetLibrary.hpp"

ByteMemBuf::ByteMemBuf(char* p, size_t l) {
    setg(p, p, p + l);

}

ByteMemStream::ByteMemStream(char* p, size_t l):
    std::istream(&buffer),
    buffer(p, l)
{
    rdbuf(&buffer);
}

std::vector<std::shared_ptr<std::stringstream>> fragment_message(std::string_view v, uint64_t stride) {
    if(v.length() <= stride)
        return {};
    uint64_t nextByte = 0;
    std::vector<std::shared_ptr<std::stringstream>> toRet;
    toRet.emplace_back(std::make_shared<std::stringstream>());
    {
        cereal::PortableBinaryOutputArchive m(*toRet.back());
        m((MessageCommandType)0, (uint64_t)v.length(), cereal::binary_data(v.data() + nextByte, stride));
    }
    nextByte += stride;
    for(;;) {
        if((v.length() - nextByte) <= stride) {
            toRet.emplace_back(std::make_shared<std::stringstream>());
            {
                cereal::PortableBinaryOutputArchive m(*toRet.back());
                m((MessageCommandType)0, cereal::binary_data(v.data() + nextByte, (uint64_t)(v.length() - nextByte)));
            }
            break;
        }
        toRet.emplace_back(std::make_shared<std::stringstream>());
        {
            cereal::PortableBinaryOutputArchive m(*toRet.back());
            m((MessageCommandType)0, cereal::binary_data(v.data() + nextByte, stride));
        }
        nextByte += stride;
    }
    return toRet;
}
