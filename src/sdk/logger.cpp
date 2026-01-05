
#include "logger.h"
#include <chrono>
#include <iomanip>
#include <sstream>

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

    // Get current time with milliseconds
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;

    localtime_r(&t, &tm);
    char timebuf[32];
    std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm);

    // Compose log line (no file/line info)
    std::ostringstream oss;
    oss << "[" << timebuf << "," << std::setfill('0') << std::setw(3) << ms.count() << "] "
        << prefix << " - " << msg;

    if (level == LogLevel::ERROR)
    {
        std::cerr << oss.str() << std::endl;
        std::cerr << std::flush;
    }
    else
    {
        std::cout << oss.str() << std::endl;
        std::cout << std::flush;
    }
}
