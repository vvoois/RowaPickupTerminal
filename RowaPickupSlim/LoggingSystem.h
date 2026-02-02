#pragma once
// LoggingSystem.h
// Logging utilities - REUSABLE in other projects
// Only depends on: string

#include <string>

namespace RowaPickupSlim::LoggingSystem
{
    /// Initialize the logging system - call this once at app startup
    /// Creates log file in the configured directory
    /// Reusable: Yes - standard initialization pattern
    void Initialize();

    /// Write a message to both the debug window and log file
    /// Message is automatically timestamped
    /// Reusable: Yes - standard logging function
    void LogMessage(const std::string& message);

    /// Clean up log files older than 31 days
    /// Call this periodically to prevent log directory from growing too large
    /// Reusable: Yes - standard cleanup pattern
    void CleanupOldLogFiles();

    /// Get the path to the current log file
    /// Reusable: Yes - standard path retrieval
    std::string GetLogFilePath();

} // namespace RowaPickupSlim::LoggingSystem


