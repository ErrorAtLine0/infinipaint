#include "MainServer.hpp"
#include "steam/steamnetworkingsockets.h"
#include "steam/steamnetworkingtypes.h"
#include <queue>
#include <string>
#include <thread>

std::mutex inputMutex;
std::queue<std::string> inputQueue;

std::unique_ptr<std::thread> inputThread;

void user_input_init() {
    inputThread = std::make_unique<std::thread>([&]() {
        for(;;) {
            std::string newMessage;
            std::cin >> newMessage;
            inputMutex.lock();
            inputQueue.emplace(newMessage);
            inputMutex.unlock();
            if(newMessage == "stop")
                break;
        }
    });
}

int main(int argc, char** argv) {
    SteamDatagramErrMsg errMsg;
    if(!GameNetworkingSockets_Init(nullptr, errMsg))
        std::cout << "[GameNetworkingSockets_Init] Failed to initialize network sockets " << errMsg << std::endl;

    MainServer server("");
    user_input_init();
    for(;;) {
        server.update();
        inputMutex.lock();
        bool isQuitting = false;
        while(!inputQueue.empty()) {
            std::string newMessage = inputQueue.front();
            inputQueue.pop();
            if(newMessage == "stop") {
                isQuitting = true;
                break;
            }
            else {
                std::cout << "Unknown command: " << newMessage << std::endl;
            }
        }
        inputMutex.unlock();
        if(isQuitting)
            break;
    }
    inputThread->join();
    return 0;
}
