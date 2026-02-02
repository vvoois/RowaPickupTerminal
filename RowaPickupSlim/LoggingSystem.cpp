// LoggingSystem.cpp
// Logging system implementation

#include "LoggingSystem.h"
#include <fstream>
#include <iostream>
#include <windows.h>
#include <shlobj.h>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace RowaPickupSlim::LoggingSystem
{
    static std::string g_logFilePath;
    static std::string g_logDirectory;
    static bool g_initialized = false;

    void Initialize()
    {
        if (g_initialized) return;

        // Create log directory: C:\ProgramData\RowaPickup\Protocol
        char appDataPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, appDataPath)))
        {
            g_logDirectory = std::string(appDataPath) + "\\RowaPickup\\Protocol";
            
            // Create directory if it doesn't exist
            CreateDirectoryA(g_logDirectory.c_str(), NULL);
            
            // Create log file path with timestamp
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            std::tm tm{};
            localtime_s(&tm, &time);
            
            std::ostringstream filename;
            filename << g_logDirectory << "\\"
                     << std::put_time(&tm, "%Y%m%d_%H%M%S")
                     << ".log";
            
            g_logFilePath = filename.str();
            g_initialized = true;
        }
    }

    void LogMessage(const std::string& message)
    {
        if (!g_initialized) return;

        // Get current timestamp
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        localtime_s(&tm, &time);

        std::ostringstream formattedMessage;
        formattedMessage << "["
                        << std::put_time(&tm, "%H:%M:%S")
                        << "] "
                        << message;

        // Write to file
        std::ofstream logFile(g_logFilePath, std::ios::app);
        if (logFile.is_open())
        {
            logFile << formattedMessage.str() << std::endl;
            logFile.close();
        }

        // Also write to debug output
        OutputDebugStringA((formattedMessage.str() + "\n").c_str());
    }

    void CleanupOldLogFiles()
    {
        if (!g_initialized) return;

        // Get current time
        auto now = std::chrono::system_clock::now();
        auto cutoffTime = now - std::chrono::hours(31 * 24);  // 31 days

        WIN32_FIND_DATAA findData;
        HANDLE findHandle = FindFirstFileA((g_logDirectory + "\\*.log").c_str(), &findData);

        if (findHandle == INVALID_HANDLE_VALUE) return;

        do
        {
            // Skip directories
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

            // Convert Windows filetime to chrono time_point
            FILETIME ftWrite = findData.ftLastWriteTime;
            ULARGE_INTEGER uli;
            uli.LowPart = ftWrite.dwLowDateTime;
            uli.HighPart = ftWrite.dwHighDateTime;

            // Windows FILETIME is 100-nanosecond intervals since 1601
            auto fileTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::duration<long long>(uli.QuadPart * 100)
            );
            
            auto fileTimePoint = std::chrono::time_point<std::chrono::system_clock>(
                std::chrono::duration_cast<std::chrono::system_clock::duration>(fileTime)
            ) - std::chrono::hours(24 * 365 + 24 * 365 / 4);  // Adjust for 1601 epoch

            // Delete if older than 31 days
            if (fileTimePoint < cutoffTime)
            {
                std::string filePath = g_logDirectory + "\\" + findData.cFileName;
                DeleteFileA(filePath.c_str());
            }
        } while (FindNextFileA(findHandle, &findData));

        FindClose(findHandle);
    }

    std::string GetLogFilePath()
    {
        return g_logFilePath;
    }

}  // namespace RowaPickupSlim::LoggingSystem



