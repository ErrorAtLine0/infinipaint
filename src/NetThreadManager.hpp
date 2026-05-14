#pragma once
#include <thread>
#include <mutex>
#include <functional>

class MainProgram;

class NetThreadManager {
    public:
        static NetThreadManager& get();
        void init(MainProgram* m);
        void thread_update();
        void destroy();
        void init_thread();
        void destroy_thread();
        void go_to_background();
        void go_to_foreground();
    private:
        static NetThreadManager global;
        MainProgram* main;
        std::mutex backgroundUpdateMutex;
        bool backgroundUpdate = false;
        std::unique_ptr<std::thread> t;
        std::atomic<bool> destroyThread = false;

        bool started = false;
};
