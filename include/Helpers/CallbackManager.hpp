#pragma once
#include <functional>
#include <vector>

template <typename... Args> class CallbackManager {
    public:
        typedef std::function<void(const Args&...)> Callback;
        Callback* register_callback(const Callback& func) {
            Callback* toRet = new Callback(func);
            funcs.emplace_back(toRet);
            return toRet;
        }
        void deregister_callback(Callback* callbackToRemove) {
            if(std::erase(funcs, callbackToRemove) > 0)
                delete callbackToRemove;
        }
        void run_callbacks(const Args&... a) {
            for(auto& f : funcs)
                (*f)(a...);
        }
        ~CallbackManager() {
            for(Callback* c : funcs)
                delete c;
        }
    private:
        std::vector<Callback*> funcs;
};
