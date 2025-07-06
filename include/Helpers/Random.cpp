#include "Random.hpp"

Random Random::global;

Random& Random::get() {
    return global;
}

std::string Random::alphanumeric_str(size_t len) {
    std::string toRet;
    char allowedChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    size_t allowedCharCount = sizeof(allowedChars) / sizeof(char);

    // We use allowedCharCount - 2 as the right bound because the range is inclusive,
    // and we dont want to include the implicit null terminator in the end of allowedChars
    // as a possible character
    for(size_t i = 0; i < len; i++)
        toRet.push_back(allowedChars[Random::get().int_range<size_t>(0, allowedCharCount - 2)]);

    return toRet;
}

Random::Random():
    randGen(std::random_device()())
{
}
