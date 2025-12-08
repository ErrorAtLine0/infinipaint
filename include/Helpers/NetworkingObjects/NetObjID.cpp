#include "NetObjID.hpp"

namespace NetworkingObjects {


NetObjID NetObjID::random_gen() {
    return NetObjID{.data = {Random::get().randGen(), Random::get().randGen()}};
}

}
