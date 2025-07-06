#include "ClientData.hpp"
#include "MainServer.hpp"
#include "CommandList.hpp"


ServerClientID ClientData::get_next_id() {
    id.second++;
    return id;
}
