#include "logger.h"

LogLevel Logger::currentLevel = LogLevel::INFO;
std::mutex Logger::logMutex;

bool Logger::isDebugEnabled()
{
    return currentLevel >= LogLevel::DEBUG;
}

void Logger::setLevel(LogLevel level)
{
    currentLevel = level;
}

void Logger::error(const std::string &msg)
{
    log("[ERROR]", msg, LogLevel::ERROR);
}

void Logger::info(const std::string &msg)
{
    log("[INFO]", msg, LogLevel::INFO);
}

void Logger::debug(const std::string &msg)
{
    log("[DEBUG]", msg, LogLevel::DEBUG);
}

void Logger::log(const std::string &prefix, const std::string &msg, LogLevel level)
{
    if (level > currentLevel)
        return;
    std::lock_guard<std::mutex> lock(logMutex);
    if (level == LogLevel::ERROR)
    {
        std::cerr << prefix << " " << msg << std::endl;
        std::cerr << std::flush;
    }
    else
    {
        std::cout << prefix << " " << msg << std::endl;
        std::cout << std::flush;
    }
}
