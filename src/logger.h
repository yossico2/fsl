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

class Logger
{
public:
    static void setLevel(LogLevel level);
    static void error(const std::string &msg);
    static void info(const std::string &msg);
    static void debug(const std::string &msg);
    static bool isDebugEnabled();

private:
    static LogLevel currentLevel;
    static std::mutex logMutex;
    static void log(const std::string &prefix, const std::string &msg, LogLevel level);
};
