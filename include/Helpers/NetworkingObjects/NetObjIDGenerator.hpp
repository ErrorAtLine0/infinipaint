#pragma once
#include <cstdint>
#include "../Hashes.hpp"

namespace NetworkingObjects {

struct NetObjID {
    typedef uint16_t ClientPortion;
    typedef uint64_t ObjectPortion;
    ClientPortion clientID = 0;
    ObjectPortion objID = 0;
    bool operator==(const NetObjID&) const = default;
    template <typename Archive> void serialize(Archive& a) {
        a(clientID, objID);
    }
};

class NetObjIDGenerator {
public:
    void set_starting_id(NetObjID c);
    NetObjID gen();
private:
    NetObjID currentID;
};

}

template <> struct std::hash<NetworkingObjects::NetObjID> {
    size_t operator()(const NetworkingObjects::NetObjID& v) const
    {
        uint64_t h = 0;
        hash_combine(h, v.clientID, v.objID);
        return h;
    }
};
