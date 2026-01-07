#pragma once
#include <cstdint>
#include <stack>
#include <Helpers/Hashes.hpp>

namespace GUIStuff {

struct GUIManagerID {
    GUIManagerID() = default;
    GUIManagerID(int64_t numID);
    GUIManagerID(const char* strID);
    bool operator==(const GUIManagerID& other) const;

    bool idIsStr;
    intptr_t idNum;
};

typedef std::vector<GUIManagerID> GUIManagerIDStack;

}

template <> struct std::hash<GUIStuff::GUIManagerID> {
    size_t operator()(const GUIStuff::GUIManagerID& v) const
    {
        return std::hash<intptr_t>{}(v.idNum);
    }
};

template <> struct std::hash<GUIStuff::GUIManagerIDStack> {
    size_t operator()(const GUIStuff::GUIManagerIDStack& v) const
    {
        uint64_t h = 0;
        for(const auto& i : v)
            hash_combine(h, i);
        return h;
    }
};
