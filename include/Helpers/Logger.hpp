#pragma once
#include <string>
#include <functional>
#include <mutex>

class Logger {
    public:
        static Logger& get();
        void add_log(const std::string& log, const std::function<void(const std::string&)> callback);
        void log(const std::string& log, const std::string& text);
        void cross_platform_println(const std::string& text);
    private:
        static Logger global;
        std::mutex logMutex;
        std::unordered_map<std::string, std::function<void(const std::string&)>> logs;
};
