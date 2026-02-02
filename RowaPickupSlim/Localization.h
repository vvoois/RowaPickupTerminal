#pragma once
// Localization.h - Multi-language UI string management
#include <string>
#include <map>
#include <vector>
#include <windows.h>

namespace RowaPickupSlim
{
    // String IDs for all UI text - use these as array indices
    enum StringID
    {
        // Connection status
        STR_CONNECTED,
        STR_NOT_CONNECTED,
        STR_TITLE_CONNECTED,
        STR_DEVICES_TITLE,
        STR_DEVICE_STATE,
        STR_MOSAIC_READY,
        STR_MOSAIC_INACTIVE,
        
        // UI Controls
        STR_SEARCH_PLACEHOLDER,
        STR_REFRESH_BUTTON,
        STR_SETTINGS_BUTTON,
        STR_OUTPUT_BUTTON,
        STR_EXIT_BUTTON,
        
        // Column headers
        STR_COL_QUANTITY,
        STR_COL_ARTICLE,
        
        // Status messages
        STR_OUR_REQUEST_LOADING,
        STR_OTHER_REQUEST_LOADING,
        STR_PICKER_LOADING,
        STR_PICKER_FAILED,
        STR_PICKER_PARTIAL,
        STR_PICKER_PARTIAL_FULL,  // "Picked: X / Y packages"
        
        // Dialogs
        STR_CONFIRM_OUTPUT,
        STR_CONFIRM_OUTPUT_TITLE,
        STR_SETTINGS_TITLE,
        STR_SETTINGS_SAVED,
        STR_SETTINGS_SAVED_TITLE,
        STR_NO_ARTICLE_SELECTED,
        STR_NO_ARTICLE_INFO,
        
        // Settings dialog
        STR_SETTINGS_IP_ADDRESS,
        STR_SETTINGS_PORT,
        STR_SETTINGS_STOCK_LOCATION,
        STR_SETTINGS_SCAN_OUTPUT,
        STR_SETTINGS_OUTPUT_NUMBER,
        STR_SETTINGS_READ_SPEED,
        STR_SETTINGS_PRIORITY,
        STR_SETTINGS_LANGUAGE,
        STR_SETTINGS_SAVE,
        STR_SETTINGS_CLOSE,
        
        // Priority options
        STR_PRIORITY_LOWEST,
        STR_PRIORITY_LOW,
        STR_PRIORITY_NORMAL,
        STR_PRIORITY_HIGH,
        STR_PRIORITY_HIGHEST,
        
        // Right-click menu
        STR_MENU_SETTINGS,
        STR_MENU_OUTPUT_SELECTED,
        STR_MENU_REFRESH_STOCK,
        STR_MENU_EXIT,
        
        // File menu
        STR_FILE_MENU,
        STR_FILE_SETTINGS,
        STR_FILE_EXIT,
        
        STR_TOTAL  // Keep this last - used for array size
    };

    // Localization manager class
    class Localization
    {
    public:
        // Get a localized string by ID
        static std::wstring GetString(StringID id);
        
        // Load language from file
        static bool LoadLanguage(const std::string& languageName);
        
        // Get list of available languages in locale folder
        static std::vector<std::string> GetAvailableLanguages();
        
        // Get current language name
        static std::string GetCurrentLanguage();
        
        // Initialize with default language
        static void Initialize(const std::string& defaultLanguage = "English");
        
        // Notify UI that language has changed - broadcasts WM_APP_LANGUAGE_CHANGED to all windows
        static void NotifyUILanguageChanged();

    private:
        static std::map<StringID, std::wstring> g_strings;
        static std::string g_currentLanguage;
        static std::string g_localeFolder;
        
        // Helper to convert UTF-8 to wide string
        static std::wstring Utf8ToWstring(const std::string& utf8);
        
        // Parse .lng file and populate strings map
        static bool ParseLanguageFile(const std::string& filePath);
    };
}
