#pragma once
#include <iostream>
#include <string>
#include <mutex>

enum class LogLevel
{
    ERROR,
    INFO,
    DEBUG
};

// Logger: Thread-safe logging utility
// Usage: Logger::setLevel(), Logger::error(), Logger::info(), Logger::debug()
class Logger
{
public:
    // Set the global logging level
    static void setLevel(LogLevel level);

    // Log an error message
    static void error(const std::string &msg);

    // Log an info message
    static void info(const std::string &msg);

    // Log a debug message (only if debug enabled)
    static void debug(const std::string &msg);

    // Returns true if debug logging is enabled
    static bool isDebugEnabled();

private:
    static LogLevel currentLevel;
    static std::mutex logMutex;
    // Internal log function
    static void log(const std::string &prefix, const std::string &msg, LogLevel level);
};
