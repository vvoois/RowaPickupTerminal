// SharedVariables.cpp
// Implementation of SharedVariables and SettingsLoader
// Loads configuration from RowaPickup.config file in CommonApplicationData\RowaPickup

#include "SharedVariables.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <random>
#include <cctype>
#include <windows.h>
#include <shlobj.h>

namespace RowaPickupSlim
{
    // Initialize static members with default values
    int SharedVariables::SelectedPrioItem = 2;
    std::string SharedVariables::SelectedPrioItemText = "Normal";
    int SharedVariables::SourceNumber = 100;
    bool SharedVariables::IsPickupsOnlyChecked = true;
    std::string SharedVariables::ClientIpAddress = "127.0.0.1";
    int SharedVariables::ClientPort = 6050;
    std::string SharedVariables::RobotStockLocation = "";
    bool SharedVariables::ScanOutput = false;
    std::string SharedVariables::ReadSpeed = "100";
    std::string SharedVariables::OutputNumber = "001";
    std::string SharedVariables::TenantId = "";  // Empty by default (optional for multi-tenant)

    // Initialize centralized AppDataFolder: C:\ProgramData\RowaPickup
    std::string SharedVariables::AppDataFolder = []() {
        char path[MAX_PATH] = { 0 };
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_COMMON_APPDATA, NULL, 0, path)))
        {
            return std::string(path) + "\\RowaPickup";
        }
        return std::string("C:\\ProgramData\\RowaPickup");  // Fallback
    }();

    // Language preference: default to Dutch
    std::string SharedVariables::Language = "NL";

    // Utility: trim whitespace from both ends of a string
    static std::string trim(const std::string& str)
    {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

    // Utility: convert string to lowercase
    static std::string to_lower(const std::string& str)
    {
        std::string s = str;
        std::transform(s.begin(), s.end(), s.begin(), 
                       [](unsigned char c){ return std::tolower(c); });
        return s;
    }

    // Parse boolean value from string (mimics C# Convert.ToBoolean)
    bool SettingsLoader::ParseBool(const std::string& value)
    {
        std::string lower = to_lower(trim(value));
        if (lower == "true" || lower == "1" || lower == "yes") return true;
        if (lower == "false" || lower == "0" || lower == "no") return false;
        return false;
    }

    // Get the config file path: CommonApplicationData\RowaPickup\RowaPickupMaui.config
    std::string SettingsLoader::GetConfigFilePath()
    {
        // Ensure the RowaPickup directory exists
        CreateDirectoryA(SharedVariables::AppDataFolder.c_str(), NULL);
        return SharedVariables::AppDataFolder + "\\RowaPickup.config";
    }

    // Load settings from config file
    bool SettingsLoader::LoadSettings()
    {
        std::string configPath = GetConfigFilePath();

        std::ifstream configFile(configPath);
        if (!configFile.is_open())
        {
            // Config file not found, use defaults
            OutputDebugStringA("Config file not found. Using default settings.\n");
            return false;
        }

        try
        {
            std::string line;
            while (std::getline(configFile, line))
            {
                // Skip empty lines and comments
                if (line.empty() || line[0] == ';' || line[0] == '#')
                    continue;

                // Parse key=value
                size_t eqPos = line.find('=');
                if (eqPos == std::string::npos)
                    continue;

                std::string key = trim(line.substr(0, eqPos));
                std::string value = trim(line.substr(eqPos + 1));

                // Process each setting
                if (key == "ClientIpAddress")
                {
                    SharedVariables::ClientIpAddress = value;
                }
                else if (key == "ClientPort")
                {
                    try { SharedVariables::ClientPort = std::stoi(value); }
                    catch (...) { /* keep default */ }
                }
                else if (key == "RobotStockLocation")
                {
                    SharedVariables::RobotStockLocation = value;
                }
                else if (key == "PickupsOnly")
                {
                    SharedVariables::IsPickupsOnlyChecked = ParseBool(value);
                }
                else if (key == "ScanOutput")
                {
                    SharedVariables::ScanOutput = ParseBool(value);
                }
                else if (key == "ReadSpeed")
                {
                    SharedVariables::ReadSpeed = value;
                }
                else if (key == "OutputNumber")
                {
                    SharedVariables::OutputNumber = value;
                    // Also generate SourceNumber from OutputNumber
                    try
                    {
                        int settingInt = std::stoi(value);
                        // Generate a random three-digit number
                        std::random_device rd;
                        std::mt19937 gen(rd());
                        std::uniform_int_distribution<> dis(100, 999);
                        int randomNumber = dis(gen);
                        // Combine to form SourceNumber
                        SharedVariables::SourceNumber = settingInt * 10000 + randomNumber;
                    }
                    catch (...) { /* keep default */ }
                }
                else if (key == "PrioPicker")
                {
                    try { SharedVariables::SelectedPrioItem = std::stoi(value); }
                    catch (...) { /* keep default */ }
                }
                else if (key == "PrioPickerText")
                {
                    SharedVariables::SelectedPrioItemText = value;
                }
                else if (key == "Language")
                {
                    SharedVariables::Language = value;
                }
                else if (key == "TenantId")
                {
                    SharedVariables::TenantId = value;
                }
            }
            configFile.close();
            return true;
        }
        catch (const std::exception& ex)
        {
            std::string msg = std::string("Error reading settings: ") + ex.what() + "\n";
            OutputDebugStringA(msg.c_str());
            return false;
        }
    }

} // namespace RowaPickupSlim
