// Localization.cpp - Multi-language UI string management implementation
#define _CRT_SECURE_NO_WARNINGS
#include "Localization.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <shlobj.h>

namespace RowaPickupSlim
{
    // Static member initialization
    std::map<StringID, std::wstring> Localization::g_strings;
    std::string Localization::g_currentLanguage;
    std::string Localization::g_localeFolder;

    // Convert UTF-8 to wide string
    std::wstring Localization::Utf8ToWstring(const std::string& utf8)
    {
        if (utf8.empty()) return L"";
        int needed = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), NULL, 0);
        std::wstring wstr(needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), (int)utf8.size(), &wstr[0], needed);
        return wstr;
    }

    // Helper to trim whitespace
    static std::string Trim(const std::string& str)
    {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

    // Map string names to enum IDs
    static StringID StringNameToID(const std::string& name)
    {
        static const std::map<std::string, StringID> nameMap = {
            { "STR_CONNECTED", STR_CONNECTED },
            { "STR_NOT_CONNECTED", STR_NOT_CONNECTED },
            { "STR_TITLE_CONNECTED", STR_TITLE_CONNECTED },
            { "STR_DEVICES_TITLE", STR_DEVICES_TITLE },
            { "STR_DEVICE_STATE", STR_DEVICE_STATE },
            { "STR_MOSAIC_READY", STR_MOSAIC_READY },
            { "STR_MOSAIC_INACTIVE", STR_MOSAIC_INACTIVE },
            
            { "STR_SEARCH_PLACEHOLDER", STR_SEARCH_PLACEHOLDER },
            { "STR_REFRESH_BUTTON", STR_REFRESH_BUTTON },
            { "STR_SETTINGS_BUTTON", STR_SETTINGS_BUTTON },
            { "STR_OUTPUT_BUTTON", STR_OUTPUT_BUTTON },
            { "STR_EXIT_BUTTON", STR_EXIT_BUTTON },
            
            { "STR_COL_QUANTITY", STR_COL_QUANTITY },
            { "STR_COL_ARTICLE", STR_COL_ARTICLE },
            
            { "STR_OUR_REQUEST_LOADING", STR_OUR_REQUEST_LOADING },
            { "STR_OTHER_REQUEST_LOADING", STR_OTHER_REQUEST_LOADING },
            { "STR_PICKER_LOADING", STR_PICKER_LOADING },
            { "STR_PICKER_FAILED", STR_PICKER_FAILED },
            { "STR_PICKER_PARTIAL", STR_PICKER_PARTIAL },
            
            { "STR_CONFIRM_OUTPUT", STR_CONFIRM_OUTPUT },
            { "STR_CONFIRM_OUTPUT_TITLE", STR_CONFIRM_OUTPUT_TITLE },
            { "STR_SETTINGS_TITLE", STR_SETTINGS_TITLE },
            { "STR_SETTINGS_SAVED", STR_SETTINGS_SAVED },
            { "STR_SETTINGS_SAVED_TITLE", STR_SETTINGS_SAVED_TITLE },
            { "STR_NO_ARTICLE_SELECTED", STR_NO_ARTICLE_SELECTED },
            { "STR_NO_ARTICLE_INFO", STR_NO_ARTICLE_INFO },
            
            { "STR_SETTINGS_IP_ADDRESS", STR_SETTINGS_IP_ADDRESS },
            { "STR_SETTINGS_PORT", STR_SETTINGS_PORT },
            { "STR_SETTINGS_STOCK_LOCATION", STR_SETTINGS_STOCK_LOCATION },
            { "STR_SETTINGS_SCAN_OUTPUT", STR_SETTINGS_SCAN_OUTPUT },
            { "STR_SETTINGS_OUTPUT_NUMBER", STR_SETTINGS_OUTPUT_NUMBER },
            { "STR_SETTINGS_READ_SPEED", STR_SETTINGS_READ_SPEED },
            { "STR_SETTINGS_PRIORITY", STR_SETTINGS_PRIORITY },
            { "STR_SETTINGS_LANGUAGE", STR_SETTINGS_LANGUAGE },
            { "STR_SETTINGS_SAVE", STR_SETTINGS_SAVE },
            { "STR_SETTINGS_CLOSE", STR_SETTINGS_CLOSE },
            
            { "STR_PRIORITY_LOWEST", STR_PRIORITY_LOWEST },
            { "STR_PRIORITY_LOW", STR_PRIORITY_LOW },
            { "STR_PRIORITY_NORMAL", STR_PRIORITY_NORMAL },
            { "STR_PRIORITY_HIGH", STR_PRIORITY_HIGH },
            { "STR_PRIORITY_HIGHEST", STR_PRIORITY_HIGHEST },
            
            { "STR_MENU_SETTINGS", STR_MENU_SETTINGS },
            { "STR_MENU_OUTPUT_SELECTED", STR_MENU_OUTPUT_SELECTED },
            { "STR_MENU_REFRESH_STOCK", STR_MENU_REFRESH_STOCK },
            { "STR_MENU_EXIT", STR_MENU_EXIT },
            
            { "STR_FILE_MENU", STR_FILE_MENU },
            { "STR_FILE_SETTINGS", STR_FILE_SETTINGS },
            { "STR_FILE_EXIT", STR_FILE_EXIT },
        };

        auto it = nameMap.find(name);
        if (it != nameMap.end())
            return it->second;
        return STR_TOTAL;  // Invalid ID
    }

    // Parse .lng file format: KEY=VALUE (UTF-8 encoded)
    bool Localization::ParseLanguageFile(const std::string& filePath)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
            return false;

        g_strings.clear();
        std::string line;
        int lineNum = 0;

        while (std::getline(file, line))
        {
            lineNum++;
            
            // Skip comments and empty lines
            if (line.empty() || line[0] == ';' || line[0] == '#')
                continue;

            // Parse KEY=VALUE
            size_t eqPos = line.find('=');
            if (eqPos == std::string::npos)
                continue;

            std::string key = Trim(line.substr(0, eqPos));
            std::string value = Trim(line.substr(eqPos + 1));

            // Map key to StringID
            StringID id = StringNameToID(key);
            if (id == STR_TOTAL)
                continue;  // Unknown key, skip

            // Store as wide string
            g_strings[id] = Utf8ToWstring(value);
        }

        file.close();
        return true;
    }

    // Get localized string
    std::wstring Localization::GetString(StringID id)
    {
        auto it = g_strings.find(id);
        if (it != g_strings.end())
            return it->second;
        
        // Fallback: return English-like placeholder if not found
        return L"[STR_" + std::to_wstring((int)id) + L"]";
    }

    // Get available languages from locale folder
    std::vector<std::string> Localization::GetAvailableLanguages()
    {
        std::vector<std::string> languages;

        try
        {
            namespace fs = std::filesystem;
            
            // Get executable directory and build locale path
            char exePath[MAX_PATH];
            GetModuleFileNameA(NULL, exePath, MAX_PATH);
            std::string exeDir = std::string(exePath);
            exeDir = exeDir.substr(0, exeDir.find_last_of("\\/"));
            std::string localeDir = exeDir + "\\locale";

            // Scan locale folder for .lng files
            if (fs::exists(localeDir))
            {
                for (const auto& entry : fs::directory_iterator(localeDir))
                {
                    if (entry.is_regular_file())
                    {
                        std::string filename = entry.path().filename().string();
                        std::string ext = entry.path().extension().string();
                        
                        // Check if .lng file
                        if (ext == ".lng" || ext == ".LNG")
                        {
                            // Remove extension to get language name
                            std::string langName = filename.substr(0, filename.find_last_of("."));
                            languages.push_back(langName);
                        }
                    }
                }
            }
        }
        catch (...)
        {
            // If error occurs, return empty list
        }

        // Sort alphabetically
        std::sort(languages.begin(), languages.end());
        return languages;
    }

    // Load language from file
    bool Localization::LoadLanguage(const std::string& languageName)
    {
        // Build path: executable_dir\locale\LanguageName.lng
        char exePath[MAX_PATH];
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string exeDir = std::string(exePath);
        exeDir = exeDir.substr(0, exeDir.find_last_of("\\/"));
        
        std::string filePath = exeDir + "\\locale\\" + languageName + ".lng";

        if (ParseLanguageFile(filePath))
        {
            g_currentLanguage = languageName;
            NotifyUILanguageChanged();  // Broadcast language change to all windows
            return true;
        }

        return false;
    }

    // Notify all windows that language has changed (broadcasts WM_APP_LANGUAGE_CHANGED)
    void Localization::NotifyUILanguageChanged()
    {
        // Find and notify the main application window
        HWND hMainWindow = FindWindowW(L"RowaPickupMainWindowClass", NULL);
        if (hMainWindow && IsWindow(hMainWindow))
        {
            // Send custom message to main window (WM_APP + 2 = WM_APP_LANGUAGE_CHANGED)
            PostMessage(hMainWindow, WM_APP + 2, 0, 0);
        }
        
        // Find and notify the settings dialog if it's open
        HWND hSettingsWindow = FindWindowW(L"RowaPickupSettingsDialog", NULL);
        if (hSettingsWindow && IsWindow(hSettingsWindow))
        {
            // Send custom message to settings dialog
            PostMessage(hSettingsWindow, WM_APP + 2, 0, 0);
        }
    }

    // Get current language name
    std::string Localization::GetCurrentLanguage()
    {
        return g_currentLanguage;
    }

    // Initialize with default language
    void Localization::Initialize(const std::string& defaultLanguage)
    {
        // Try to load default language
        if (!LoadLanguage(defaultLanguage))
        {
            // If default fails, try to load any available language
            auto available = GetAvailableLanguages();
            if (!available.empty())
            {
                LoadLanguage(available[0]);
            }
        }
    }
}
