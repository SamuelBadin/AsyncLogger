#include "AsyncLogger.h"
#include <thread>
#include <chrono>

int main() {
	AsyncLogger logger("logs/logfile",
		LogLevel::DEBUG,
		true,   // Separate file for each log level
		false,  // Use plain text instead of JSON
		1024 * 1024); // Max file size (not yet used)

	// Log various messages
	logger.log(LogLevel::DEBUG, "This is a debug message");
	logger.log(LogLevel::INFO, "This is an info message");
	logger.log(LogLevel::WARNING, "This is a warning message");
	logger.log(LogLevel::ERROR, "This is an error message");

	// Sleep for a moment to allow background thread to flush logs
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	return 0;
}
