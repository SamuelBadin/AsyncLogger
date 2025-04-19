#include "AsyncLogger.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <filesystem>

// Constructor for LogMessage: captures timestamp
LogMessage::LogMessage(LogLevel lvl, std::string msg)
    : level(lvl), message(std::move(msg)) {
    auto now = std::chrono::system_clock::now();
    auto in_time = std::chrono::system_clock::to_time_t(now);
    std::tm buf{};
#ifdef _WIN32
    localtime_s(&buf, &in_time);
#else
    localtime_r(&in_time, &buf);
#endif
    std::ostringstream ss;
    ss << std::put_time(&buf, "%Y-%m-%d %H:%M:%S");
    timestamp = ss.str();
}

// Helper function to convert enum to string
static std::string logLevelToString(LogLevel level) {
    switch (level) {
    case LogLevel::DEBUG:   return "DEBUG";
    case LogLevel::INFO:    return "INFO";
    case LogLevel::WARNING: return "WARNING";
    case LogLevel::ERROR:   return "ERROR";
    default: return "UNKNOWN";
    }
}

// Logger constructor: initializes worker thread
AsyncLogger::AsyncLogger(const std::string& baseFilename,
    LogLevel minLogLevel,
    bool separateFilesByLevel,
    bool useJsonFormat,
    size_t maxFileSizeBytes)
    : baseFilename(baseFilename),
    minLogLevel(minLogLevel),
    separateFilesByLevel(separateFilesByLevel),
    useJsonFormat(useJsonFormat),
    maxFileSizeBytes(maxFileSizeBytes),
    exitFlag(false)
{
    // Ensure output directory exists
    if (!std::filesystem::exists(std::filesystem::path(baseFilename).parent_path())) {
        std::filesystem::create_directories(std::filesystem::path(baseFilename).parent_path());
    }

    // Launch worker thread
    worker = std::thread(&AsyncLogger::processQueue, this);
}

// Logger destructor: shuts down worker thread and closes files
AsyncLogger::~AsyncLogger() {
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        exitFlag = true;
    }
    cv.notify_one();

    if (worker.joinable()) {
        worker.join();
    }

    for (auto& [_, file] : files) {
        if (file && file->is_open()) {
            file->close();
        }
    }
}

// Queues a message to be logged asynchronously
void AsyncLogger::log(LogLevel level, const std::string& message) {
    if (level < minLogLevel) return;
    auto logMsg = std::make_unique<LogMessage>(level, message);

    {
        std::lock_guard<std::mutex> lock(queueMutex);
        logQueue.emplace(std::move(logMsg));
    }
    cv.notify_one();
}

// Main logging loop running in a separate thread
void AsyncLogger::processQueue() {
    while (true) {
        std::unique_ptr<LogMessage> msg;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            cv.wait(lock, [this]() { return !logQueue.empty() || exitFlag; });

            if (exitFlag && logQueue.empty()) break;

            msg = std::move(logQueue.front());
            logQueue.pop();
        }

        std::string formatted = formatMessage(*msg);
        outputToConsole(*msg, formatted);
        outputToFile(*msg, formatted);
    }
}

// Formats the message to either JSON or human-readable text
std::string AsyncLogger::formatMessage(const LogMessage& msg) {
    if (useJsonFormat) {
        std::ostringstream ss;
        ss << "{\"timestamp\":\"" << msg.timestamp
            << "\",\"level\":\"" << logLevelToString(msg.level)
            << "\",\"message\":\"" << msg.message << "\"}";
        return ss.str();
    }
    else {
        std::ostringstream ss;
        ss << msg.timestamp << " [" << logLevelToString(msg.level) << "] " << msg.message;
        return ss.str();
    }
}

// Prints the message to the console (with color on non-Windows systems)
void AsyncLogger::outputToConsole(const LogMessage& msg, const std::string& formatted) {
#ifdef _WIN32
    std::cout << formatted << std::endl;
#else
    std::string colorCode;
    switch (msg.level) {
    case LogLevel::DEBUG:   colorCode = "\033[90m"; break;
    case LogLevel::INFO:    colorCode = "\033[32m"; break;
    case LogLevel::WARNING: colorCode = "\033[33m"; break;
    case LogLevel::ERROR:   colorCode = "\033[31m"; break;
    }
    std::cout << colorCode << formatted << "\033[0m" << std::endl;
#endif
}

// Appends the message to a log file
void AsyncLogger::outputToFile(const LogMessage& msg, const std::string& formatted) {
    LogLevel level = separateFilesByLevel ? msg.level : LogLevel::DEBUG;

    // Open the file if not already open
    if (!files[level]) {
        std::string filename = baseFilename;
        if (separateFilesByLevel) {
            filename += "_" + logLevelToString(level);
        }
        filename += ".log";

        files[level] = std::make_unique<std::ofstream>(filename, std::ios::app);
        fileIndex[level] = 1;
    }

    auto& file = files[level];
    if (file && file->is_open()) {
        *file << formatted << std::endl;
        file->flush();
    }
}
