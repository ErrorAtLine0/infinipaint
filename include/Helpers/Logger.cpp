#include "Logger.hpp"
#include <stdexcept>

Logger Logger::global;

Logger& Logger::get() {
    return global;
}

void Logger::add_log(const std::string& log, const std::function<void(const std::string&)> callback) {
    std::scoped_lock logLock(logMutex);
    logs[log] = callback;
}

void Logger::log(const std::string& log, const std::string& text) {
    std::scoped_lock logLock(logMutex);
    auto it = logs.find(log);
    if(it == logs.end())
        throw std::runtime_error("[Logger::log] Log " + log + " does not exist to log text: " + text);
    else
        it->second(text);
}
