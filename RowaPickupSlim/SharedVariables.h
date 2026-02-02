#pragma once
// SharedVariables.h
// C++ port of SharedVariables.cs from the MAUI project.
// Global configuration and settings for the RowaPickup application.

#include <string>
#include <windows.h>

namespace RowaPickupSlim
{
    // Connection state machine - MOVED TO Shared.h to consolidate enums
    // NetworkConnectionState is defined in Shared.h

    // Global application settings (mirrored from C# SharedVariables)
    class SharedVariables
    {
    public:
        // Configuration properties
        static int SelectedPrioItem;
        static std::string SelectedPrioItemText;
        static int SourceNumber;
        static bool IsPickupsOnlyChecked;
        static std::string ClientIpAddress;
        static int ClientPort;
        static std::string RobotStockLocation;
        static bool ScanOutput;
        static std::string ReadSpeed;
        static std::string OutputNumber;
        static std::string TenantId;  // Optional tenant ID for multi-tenant systems

        // Language preference: "NL" for Dutch, "EN" for English
        static std::string Language;

        // Centralized application data folder path: C:\ProgramData\RowaPickup
        static std::string AppDataFolder;

    private:
        SharedVariables() = delete; // static class, no instances
    };

    // Simple data class for temporary article info
    struct TemporaryArticle
    {
        std::string Id;
        std::string Name;
        std::string PackagingUnit;
        std::string DosageForm;
        int Quantity = 0;
    };

    // Settings loader: read from config file and populate SharedVariables
    class SettingsLoader
    {
    public:
        // Load settings from config file (synchronous, replaces async C# version)
        static bool LoadSettings();

    private:
        SettingsLoader() = delete; // static class, no instances
        static std::string GetConfigFilePath();
        static bool ParseBool(const std::string& value);
    };

} // namespace RowaPickupSlim
