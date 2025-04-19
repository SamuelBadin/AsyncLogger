#pragma once

#include <string>
#include <fstream>
#include <map>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <memory>
#include <atomic>

// Enum representing log levels
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

// Structure to hold a log message
struct LogMessage {
    LogLevel level;
    std::string message;
    std::string timestamp;

    LogMessage(LogLevel lvl, std::string msg);
};

// Asynchronous logger class
class AsyncLogger {
public:
    AsyncLogger(const std::string& baseFilename,
        LogLevel minLogLevel = LogLevel::DEBUG,
        bool separateFilesByLevel = true,
        bool useJsonFormat = false,
        size_t maxFileSizeBytes = 1024 * 1024);

    ~AsyncLogger();

    void log(LogLevel level, const std::string& message);

private:
    std::string baseFilename;
    LogLevel minLogLevel;
    bool separateFilesByLevel;
    bool useJsonFormat;
    size_t maxFileSizeBytes;

    std::map<LogLevel, std::unique_ptr<std::ofstream>> files;
    std::map<LogLevel, int> fileIndex;

    std::mutex queueMutex;
    std::condition_variable cv;
    std::queue<std::unique_ptr<LogMessage>> logQueue;
    std::thread worker;
    std::atomic<bool> exitFlag;

    void processQueue();
    std::string formatMessage(const LogMessage& msg);
    void outputToConsole(const LogMessage& msg, const std::string& formatted);
    void outputToFile(const LogMessage& msg, const std::string& formatted);
};
