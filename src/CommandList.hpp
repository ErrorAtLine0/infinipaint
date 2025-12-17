#pragma once
#include <cstdint>
#include <Helpers/Networking/NetLibrary.hpp>

enum ServerCommands : MessageCommandType {
    SERVER_FRAGMENT = 0,
    SERVER_KEEP_ALIVE,
    SERVER_INITIAL_DATA,
    SERVER_UPDATE_NETWORK_OBJECT,
    SERVER_NEW_RESOURCE_ID,
    SERVER_NEW_RESOURCE_DATA
};

enum ClientCommands : MessageCommandType {
    CLIENT_FRAGMENT = 0,
    CLIENT_KEEP_ALIVE,
    CLIENT_INITIAL_DATA,
    CLIENT_UPDATE_NETWORK_OBJECT,
    CLIENT_NEW_RESOURCE_ID,
    CLIENT_NEW_RESOURCE_DATA
};

// Arbitrary value. Higher scale up values lead to less scale up events (which can cause stuttering), but leads to larger numbers overall, which can cause a tiny bit of lag and increased memory usage
#define CANVAS_SCALE_UP_STEP WorldScalar("100000000000000000000000000000000000000000000000000")
