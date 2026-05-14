#include "NetThreadManager.hpp"
#include <Helpers/Networking/NetLibrary.hpp>
#include "AndroidJNICalls.hpp"
#include "MainProgram.hpp"

NetThreadManager NetThreadManager::global;

NetThreadManager& NetThreadManager::get() {
    return global;
}

void NetThreadManager::init(MainProgram* m) {
    if(started)
        return;
    started = true;
    main = m;
#ifndef __ANDROID__
    init_thread();
#else
    AndroidJNICalls::startNetworkService();
#endif
}

void NetThreadManager::init_thread() {
    if(t)
        throw std::runtime_error("[NetThreadManager::init_thread] Thread can't exist when init_thread is called");
    destroyThread = false;
    t = std::make_unique<std::thread>([&]{ thread_update(); });
}

void NetThreadManager::thread_update() {
    NetLibrary::init_websocket();
    for(;;) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(20ms);
        if(destroyThread) {
            NetLibrary::destroy_websocket();
            break;
        }
        {
            std::scoped_lock l{backgroundUpdateMutex};
            if(backgroundUpdate)
                main->background_update();
        }
        NetLibrary::update();
    }
#ifdef __ANDROID__
    AndroidJNICalls::stopNetworkService();
#endif
}

void NetThreadManager::destroy_thread() {
    if(!t)
        throw std::runtime_error("[NetThreadManager::init_thread] Thread can't be null when destroy_thread is called");
    destroyThread = true;
    t->join();
    t = nullptr;
}

void NetThreadManager::go_to_background() {
    std::scoped_lock l{backgroundUpdateMutex};
    backgroundUpdate = true;
}

void NetThreadManager::go_to_foreground() {
    std::scoped_lock l{backgroundUpdateMutex};
    backgroundUpdate = false;
}

void NetThreadManager::destroy() {
    if(!started)
        return;
    started = false;
#ifndef __ANDROID__
    destroy_thread();
#else
    AndroidJNICalls::stopNetworkService();
#endif
}
