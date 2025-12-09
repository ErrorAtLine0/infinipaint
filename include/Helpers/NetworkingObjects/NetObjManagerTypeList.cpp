#include "NetObjManagerTypeList.hpp"

namespace NetworkingObjects {
    const NetObjManagerTypeList::NetTypeIDFunctions& NetObjManagerTypeList::get_net_type_data(bool isServer, NetTypeIDType id) {
        return isServer ? netTypeIDFunctionsServer[id] : netTypeIDFunctionsClient[id];
    }
}
