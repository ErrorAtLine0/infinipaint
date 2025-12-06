#include "NetObjIDGenerator.hpp"

namespace NetworkingObjects {

void NetObjIDGenerator::set_starting_id(NetObjID c) {
    currentID = c;
}

NetObjID NetObjIDGenerator::gen() {
    currentID.clientID++;
    return currentID;
}

}
