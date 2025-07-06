#pragma once
#include <stack>
#include <Helpers/Hashes.hpp>

namespace GUIStuff {

struct GUIManagerID {
    GUIManagerID() = default;
    GUIManagerID(int64_t numID);
    GUIManagerID(const std::string& strID);
    bool operator==(const GUIManagerID& other) const;

    bool idIsStr;
    std::string idStr;
    int64_t idNum = 0;
};

typedef std::vector<GUIManagerID> GUIManagerIDStack;

}

template <> struct std::hash<GUIStuff::GUIManagerID> {
    size_t operator()(const GUIStuff::GUIManagerID& v) const
    {
        if(v.idIsStr)
            return std::hash<std::string>{}(v.idStr);
        else
            return std::hash<uint64_t>{}(v.idNum);
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
