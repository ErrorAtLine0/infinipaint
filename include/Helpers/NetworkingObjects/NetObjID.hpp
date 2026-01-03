#pragma once
#include <cstdint>
#include <array>
#include <Helpers/Random.hpp>
#include <Helpers/Hashes.hpp>

namespace NetworkingObjects {
    struct NetObjID {
        std::array<uint64_t, 2> data;
        template <typename Archive> void serialize(Archive& a) {
            a(data[1], data[0]);
        }
        std::string to_string() const {
            std::stringstream ss;
            ss << "[" << data[1] << ", " << data[0] << "]";
            return ss.str();
        }
        NetObjID& operator=(const NetObjID&) = default;
        bool operator==(const NetObjID&) const = default;
        static NetObjID random_gen();
    };
};

template <> struct std::hash<NetworkingObjects::NetObjID> {
    size_t operator()(const NetworkingObjects::NetObjID& v) const
    {
        uint64_t h = 0;
        hash_combine(h, v.data[1], v.data[0]);
        return h;
    }
};
