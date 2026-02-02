// SettingsDialog.cpp - Settings dialog implementation with file persistence
#define _CRT_SECURE_NO_WARNINGS
#include "SettingsDialog.h"
#include "SharedVariables.h"
#include "Localization.h"
#include <windows.h>
#include <string>
#include <commctrl.h>
#include <fstream>

#pragma comment(lib, "comctl32.lib")

namespace RowaPickupSlim
{
    // Global dialog handle (needed for window proc access)
    static HWND g_hwndSettingsDialog = NULL;
    
    // Label strings to update (map of control ID to localization string ID)
    // We'll define these when we create the dialog
    static const int LABEL_IP_ADDRESS = 2001;
    static const int LABEL_PORT = 2002;
    static const int LABEL_STOCK_LOCATION = 2003;
    static const int LABEL_SCAN_OUTPUT = 2004;
    static const int LABEL_OUTPUT_NUMBER = 2005;
    static const int LABEL_READ_SPEED = 2006;
    static const int LABEL_PRIORITY = 2007;
    static const int LABEL_LANGUAGE = 2008;
    static const int BUTTON_SAVE = 2009;
    static const int BUTTON_CLOSE = 2010;

    // Helper: Convert UTF-8 string to wide string
    static std::wstring utf8_to_wstring(const std::string& utf8)
    {
        if (utf8.empty()) return L"";
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &utf8[0], (int)utf8.size(), NULL, 0);
        std::wstring wstr(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &utf8[0], (int)utf8.size(), &wstr[0], size_needed);
        return wstr;
    }

    // Helper: Save settings to config file
    static void SaveSettingsToFile()
    {
        // Use centralized AppDataFolder path
        std::string configPath = SharedVariables::AppDataFolder + "\\RowaPickup.config";
        
        // Ensure directory exists
        CreateDirectoryA(SharedVariables::AppDataFolder.c_str(), NULL);

        std::ofstream configFile(configPath);
        if (!configFile.is_open()) return;

        configFile << "ClientIpAddress=" << SharedVariables::ClientIpAddress << "\n";
        configFile << "ClientPort=" << SharedVariables::ClientPort << "\n";
        configFile << "RobotStockLocation=" << SharedVariables::RobotStockLocation << "\n";
        configFile << "ScanOutput=" << (SharedVariables::ScanOutput ? "true" : "false") << "\n";
        configFile << "OutputNumber=" << SharedVariables::OutputNumber << "\n";
        configFile << "ReadSpeed=" << SharedVariables::ReadSpeed << "\n";
        configFile << "TenantId=" << SharedVariables::TenantId << "\n";
        configFile << "PrioPicker=" << SharedVariables::SelectedPrioItem << "\n";
        configFile << "PrioPickerText=" << SharedVariables::SelectedPrioItemText << "\n";
        configFile << "Language=" << SharedVariables::Language << "\n";
        configFile.close();
    }

    // Window procedure for settings dialog
    static LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_APP + 2:  // WM_APP_LANGUAGE_CHANGED
        {
            // Language changed - update all dialog labels
            SettingsDialog::UpdateLanguage();
            return 0;
        }
        case WM_COMMAND:
        {
            int id = LOWORD(wParam);
            int notifyCode = HIWORD(wParam);
            
            if (id == BUTTON_SAVE)  // Save button (was 1000, now using constant)
            {
                // IP Address
                HWND hIP = GetDlgItem(hwnd, 101);
                if (hIP)
                {
                    wchar_t buf[256] = {0};
                    GetWindowTextW(hIP, buf, 256);
                    char cbuf[256];
                    WideCharToMultiByte(CP_UTF8, 0, buf, -1, cbuf, sizeof(cbuf), NULL, NULL);
                    SharedVariables::ClientIpAddress = cbuf;
                }

                // Port
                HWND hPort = GetDlgItem(hwnd, 102);
                if (hPort)
                {
                    wchar_t buf[256] = {0};
                    GetWindowTextW(hPort, buf, 256);
                    SharedVariables::ClientPort = _wtoi(buf);
                }

                // Stock Location
                HWND hLoc = GetDlgItem(hwnd, 103);
                if (hLoc)
                {
                    wchar_t buf[256] = {0};
                    GetWindowTextW(hLoc, buf, 256);
                    char cbuf[256];
                    WideCharToMultiByte(CP_UTF8, 0, buf, -1, cbuf, sizeof(cbuf), NULL, NULL);
                    SharedVariables::RobotStockLocation = cbuf;
                }

                // ScanOutput checkbox
                HWND hScan = GetDlgItem(hwnd, 104);
                if (hScan)
                {
                    SharedVariables::ScanOutput = (SendMessageW(hScan, BM_GETCHECK, 0, 0) == BST_CHECKED);
                }

                // Output Number
                HWND hOut = GetDlgItem(hwnd, 105);
                if (hOut)
                {
                    wchar_t buf[256] = {0};
                    GetWindowTextW(hOut, buf, 256);
                    char cbuf[256];
                    WideCharToMultiByte(CP_UTF8, 0, buf, -1, cbuf, sizeof(cbuf), NULL, NULL);
                    SharedVariables::OutputNumber = cbuf;
                }

                // Read Speed
                HWND hSpeed = GetDlgItem(hwnd, 106);
                if (hSpeed)
                {
                    wchar_t buf[256] = {0};
                    GetWindowTextW(hSpeed, buf, 256);
                    char cbuf[256];
                    WideCharToMultiByte(CP_UTF8, 0, buf, -1, cbuf, sizeof(cbuf), NULL, NULL);
                    SharedVariables::ReadSpeed = cbuf;
                }

                // TenantId
                HWND hTenantId = GetDlgItem(hwnd, 1012);  // IDC_TENANTID = 1012
                if (hTenantId)
                {
                    wchar_t buf[256] = {0};
                    GetWindowTextW(hTenantId, buf, 256);
                    char cbuf[256];
                    WideCharToMultiByte(CP_UTF8, 0, buf, -1, cbuf, sizeof(cbuf), NULL, NULL);
                    SharedVariables::TenantId = cbuf;
                }

                // Priority
                HWND hPrio = GetDlgItem(hwnd, 107);
                if (hPrio)
                {
                    int sel = SendMessageW(hPrio, CB_GETCURSEL, 0, 0);
                    if (sel != CB_ERR)
                    {
                        SharedVariables::SelectedPrioItem = sel;
                        const char* prioText[] = {"Lowest", "Low", "Normal", "High", "Highest"};
                        if (sel >= 0 && sel < 5) SharedVariables::SelectedPrioItemText = prioText[sel];
                    }
                }

                // Language
                HWND hLang = GetDlgItem(hwnd, 108);
                if (hLang)
                {
                    int sel = SendMessageW(hLang, CB_GETCURSEL, 0, 0);
                    if (sel != CB_ERR)
                    {
                        wchar_t langName[256] = {0};
                        SendMessageW(hLang, CB_GETLBTEXT, sel, (LPARAM)langName);
                        
                        // Convert to UTF-8 for storage
                        char langBuf[256];
                        WideCharToMultiByte(CP_UTF8, 0, langName, -1, langBuf, sizeof(langBuf), NULL, NULL);
                        SharedVariables::Language = langBuf;
                        
                        // Load language immediately
                        RowaPickupSlim::Localization::LoadLanguage(langBuf);
                    }
                }

                SaveSettingsToFile();
                MessageBoxW(hwnd, RowaPickupSlim::Localization::GetString(RowaPickupSlim::STR_SETTINGS_SAVED).c_str(), 
                           RowaPickupSlim::Localization::GetString(RowaPickupSlim::STR_SETTINGS_SAVED_TITLE).c_str(), 
                           MB_OK | MB_ICONINFORMATION);
                
                // Get parent window before clearing dialog handle
                HWND hParent = GetParent(hwnd);
                
                // Restore focus to parent window before closing
                if (hParent && IsWindow(hParent))
                {
                    SetFocus(hParent);
                }
                
                // Hide instead of destroying - allows language update without recreation
                ShowWindow(hwnd, SW_HIDE);
                
                // Language change notification is now automatic via Localization::NotifyUILanguageChanged()
                
                return 0;
            }
            else if (id == BUTTON_CLOSE)  // Close button (was 1001, now using constant)
            {
                // Restore focus to parent window before closing
                HWND hParent = GetParent(hwnd);
                if (hParent && IsWindow(hParent))
                {
                    SetFocus(hParent);
                }
                
                // Hide instead of destroying - allows language update without recreation
                ShowWindow(hwnd, SW_HIDE);
                return 0;
            }
            break;
        }
        case WM_CLOSE:
        {
            // Close and destroy the dialog window
            DestroyWindow(hwnd);
            return 0;
        }
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    // Update all dialog labels with new language
    void SettingsDialog::UpdateLanguage()
    {
        if (!g_hwndSettingsDialog || !IsWindow(g_hwndSettingsDialog)) return;
        
        // Update labels with current language
        HWND hLabel = NULL;
        
        // IP Address label
        hLabel = GetDlgItem(g_hwndSettingsDialog, LABEL_IP_ADDRESS);
        if (hLabel) SetWindowTextW(hLabel, Localization::GetString(STR_SETTINGS_IP_ADDRESS).c_str());
        
        // Port label
        hLabel = GetDlgItem(g_hwndSettingsDialog, LABEL_PORT);
        if (hLabel) SetWindowTextW(hLabel, Localization::GetString(STR_SETTINGS_PORT).c_str());
        
        // Stock Location label
        hLabel = GetDlgItem(g_hwndSettingsDialog, LABEL_STOCK_LOCATION);
        if (hLabel) SetWindowTextW(hLabel, Localization::GetString(STR_SETTINGS_STOCK_LOCATION).c_str());
        
        // Scan Output label
        hLabel = GetDlgItem(g_hwndSettingsDialog, LABEL_SCAN_OUTPUT);
        if (hLabel) SetWindowTextW(hLabel, Localization::GetString(STR_SETTINGS_SCAN_OUTPUT).c_str());
        
        // Output Number label
        hLabel = GetDlgItem(g_hwndSettingsDialog, LABEL_OUTPUT_NUMBER);
        if (hLabel) SetWindowTextW(hLabel, Localization::GetString(STR_SETTINGS_OUTPUT_NUMBER).c_str());
        
        // Read Speed label
        hLabel = GetDlgItem(g_hwndSettingsDialog, LABEL_READ_SPEED);
        if (hLabel) SetWindowTextW(hLabel, Localization::GetString(STR_SETTINGS_READ_SPEED).c_str());
        
        // Priority label
        hLabel = GetDlgItem(g_hwndSettingsDialog, LABEL_PRIORITY);
        if (hLabel) SetWindowTextW(hLabel, Localization::GetString(STR_SETTINGS_PRIORITY).c_str());
        
        // Language label
        hLabel = GetDlgItem(g_hwndSettingsDialog, LABEL_LANGUAGE);
        if (hLabel) SetWindowTextW(hLabel, Localization::GetString(STR_SETTINGS_LANGUAGE).c_str());
        
        // Buttons
        HWND hBtn = GetDlgItem(g_hwndSettingsDialog, BUTTON_SAVE);
        if (hBtn) SetWindowTextW(hBtn, Localization::GetString(STR_SETTINGS_SAVE).c_str());
        
        hBtn = GetDlgItem(g_hwndSettingsDialog, BUTTON_CLOSE);
        if (hBtn) SetWindowTextW(hBtn, Localization::GetString(STR_SETTINGS_CLOSE).c_str());
        
        // Force redraw
        InvalidateRect(g_hwndSettingsDialog, NULL, TRUE);
    }

    // Create and show the settings dialog
    HWND SettingsDialog::Create(HWND hwndParent)
    {
        if (g_hwndSettingsDialog != NULL) 
        {
            ShowWindow(g_hwndSettingsDialog, SW_SHOW);
            SetFocus(g_hwndSettingsDialog);
            return g_hwndSettingsDialog;
        }

        // Register window class
        static bool classRegistered = false;
        if (!classRegistered)
        {
            WNDCLASSW wc = {};
            wc.lpfnWndProc = SettingsWndProc;
            wc.hInstance = (HINSTANCE)GetModuleHandleW(NULL);
            wc.lpszClassName = L"RowaPickupSettingsDialog";
            wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
            RegisterClassW(&wc);
            classRegistered = true;
        }

        // Create settings window
        g_hwndSettingsDialog = CreateWindowExW(
            WS_EX_DLGMODALFRAME,
            L"RowaPickupSettingsDialog",
            L"RowaPickup Settings",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 550, 500,
            hwndParent,
            NULL,
            (HINSTANCE)GetModuleHandleW(NULL),
            NULL
        );

        if (!g_hwndSettingsDialog) return NULL;

        // Create controls
        int y = 15;
        const int labelWidth = 150;
        const int ctrlWidth = 280;
        const int lineHeight = 35;
        const int xLabel = 10;
        const int xCtrl = xLabel + labelWidth;

        HINSTANCE hInst = (HINSTANCE)GetModuleHandleW(NULL);

        // IP Address
        std::wstring wIp = utf8_to_wstring(SharedVariables::ClientIpAddress);
        CreateWindowW(L"STATIC", Localization::GetString(STR_SETTINGS_IP_ADDRESS).c_str(), WS_CHILD | WS_VISIBLE, 
                        xLabel, y, labelWidth, 20, g_hwndSettingsDialog, (HMENU)(intptr_t)LABEL_IP_ADDRESS, hInst, NULL);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", wIp.c_str(),
                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                        xCtrl, y, ctrlWidth, 20, g_hwndSettingsDialog, (HMENU)(intptr_t)101, hInst, NULL);
        y += lineHeight;

        // Port
        std::wstring wPort = utf8_to_wstring(std::to_string(SharedVariables::ClientPort));
        CreateWindowW(L"STATIC", Localization::GetString(STR_SETTINGS_PORT).c_str(), WS_CHILD | WS_VISIBLE,
                        xLabel, y, labelWidth, 20, g_hwndSettingsDialog, (HMENU)(intptr_t)LABEL_PORT, hInst, NULL);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", wPort.c_str(),
                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER,
                        xCtrl, y, ctrlWidth, 20, g_hwndSettingsDialog, (HMENU)(intptr_t)102, hInst, NULL);
        y += lineHeight;

        // Stock Location
        std::wstring wLocation = utf8_to_wstring(SharedVariables::RobotStockLocation);
        CreateWindowW(L"STATIC", Localization::GetString(STR_SETTINGS_STOCK_LOCATION).c_str(), WS_CHILD | WS_VISIBLE,
                        xLabel, y, labelWidth, 20, g_hwndSettingsDialog, (HMENU)(intptr_t)LABEL_STOCK_LOCATION, hInst, NULL);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", wLocation.c_str(),
                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                        xCtrl, y, ctrlWidth, 20, g_hwndSettingsDialog, (HMENU)(intptr_t)103, hInst, NULL);
        y += lineHeight;

        // ScanOutput checkbox
        CreateWindowW(L"STATIC", Localization::GetString(STR_SETTINGS_SCAN_OUTPUT).c_str(), WS_CHILD | WS_VISIBLE,
                        xLabel, y, labelWidth, 20, g_hwndSettingsDialog, (HMENU)(intptr_t)LABEL_SCAN_OUTPUT, hInst, NULL);
        HWND hScan = CreateWindowW(L"BUTTON", L"",
                        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                        xCtrl, y, ctrlWidth, 20, g_hwndSettingsDialog, (HMENU)(intptr_t)104, hInst, NULL);
        SendMessageW(hScan, BM_SETCHECK, SharedVariables::ScanOutput ? BST_CHECKED : BST_UNCHECKED, 0);
        y += lineHeight;

        // Output Number
        std::wstring wOutput = utf8_to_wstring(SharedVariables::OutputNumber);
        CreateWindowW(L"STATIC", Localization::GetString(STR_SETTINGS_OUTPUT_NUMBER).c_str(), WS_CHILD | WS_VISIBLE,
                        xLabel, y, labelWidth, 20, g_hwndSettingsDialog, (HMENU)(intptr_t)LABEL_OUTPUT_NUMBER, hInst, NULL);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", wOutput.c_str(),
                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                        xCtrl, y, ctrlWidth, 20, g_hwndSettingsDialog, (HMENU)(intptr_t)105, hInst, NULL);
        y += lineHeight;

        // Read Speed
        std::wstring wSpeed = utf8_to_wstring(SharedVariables::ReadSpeed);
        CreateWindowW(L"STATIC", Localization::GetString(STR_SETTINGS_READ_SPEED).c_str(), WS_CHILD | WS_VISIBLE,
                        xLabel, y, labelWidth, 20, g_hwndSettingsDialog, (HMENU)(intptr_t)LABEL_READ_SPEED, hInst, NULL);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", wSpeed.c_str(),
                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER,
                        xCtrl, y, ctrlWidth, 20, g_hwndSettingsDialog, (HMENU)(intptr_t)106, hInst, NULL);
        y += lineHeight;

        // TenantId (no translation - fixed parameter name)
        std::wstring wTenantId = utf8_to_wstring(SharedVariables::TenantId);
        CreateWindowW(L"STATIC", L"TenantId", WS_CHILD | WS_VISIBLE,
                        xLabel, y, labelWidth, 20, g_hwndSettingsDialog, (HMENU)(intptr_t)(LABEL_READ_SPEED + 1), hInst, NULL);
        CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", wTenantId.c_str(),
                        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                        xCtrl, y, ctrlWidth, 20, g_hwndSettingsDialog, (HMENU)(intptr_t)1012, hInst, NULL);
        y += lineHeight;

        // Priority
        CreateWindowW(L"STATIC", Localization::GetString(STR_SETTINGS_PRIORITY).c_str(), WS_CHILD | WS_VISIBLE,
                        xLabel, y, labelWidth, 20, g_hwndSettingsDialog, (HMENU)(intptr_t)LABEL_PRIORITY, hInst, NULL);
        HWND hPrio = CreateWindowW(L"COMBOBOX", L"",
                        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                        xCtrl, y, ctrlWidth, 100, g_hwndSettingsDialog, (HMENU)(intptr_t)107, hInst, NULL);
        SendMessageW(hPrio, CB_ADDSTRING, 0, (LPARAM)L"Lowest");
        SendMessageW(hPrio, CB_ADDSTRING, 0, (LPARAM)L"Low");
        SendMessageW(hPrio, CB_ADDSTRING, 0, (LPARAM)L"Normal");
        SendMessageW(hPrio, CB_ADDSTRING, 0, (LPARAM)L"High");
        SendMessageW(hPrio, CB_ADDSTRING, 0, (LPARAM)L"Highest");
        SendMessageW(hPrio, CB_SETCURSEL, SharedVariables::SelectedPrioItem, 0);
        y += lineHeight * 2;

        // Language
        CreateWindowW(L"STATIC", Localization::GetString(STR_SETTINGS_LANGUAGE).c_str(), WS_CHILD | WS_VISIBLE,
                        xLabel, y, labelWidth, 20, g_hwndSettingsDialog, (HMENU)(intptr_t)LABEL_LANGUAGE, hInst, NULL);
        HWND hLang = CreateWindowW(L"COMBOBOX", L"",
                        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                        xCtrl, y, ctrlWidth, 100, g_hwndSettingsDialog, (HMENU)(intptr_t)108, hInst, NULL);
        
        // Populate language dropdown with available languages
        auto languages = RowaPickupSlim::Localization::GetAvailableLanguages();
        int currentLangIndex = 0;
        for (size_t i = 0; i < languages.size(); ++i)
        {
            std::wstring wLang = std::wstring(languages[i].begin(), languages[i].end());
            SendMessageW(hLang, CB_ADDSTRING, 0, (LPARAM)wLang.c_str());
            
            if (languages[i] == SharedVariables::Language)
                currentLangIndex = (int)i;
        }
        SendMessageW(hLang, CB_SETCURSEL, currentLangIndex, 0);
        y += lineHeight * 2;

        // Buttons
        CreateWindowW(L"BUTTON", Localization::GetString(STR_SETTINGS_SAVE).c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        xCtrl, y, 120, 25, g_hwndSettingsDialog, (HMENU)(intptr_t)BUTTON_SAVE, hInst, NULL);
        CreateWindowW(L"BUTTON", Localization::GetString(STR_SETTINGS_CLOSE).c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                        xCtrl + 130, y, 120, 25, g_hwndSettingsDialog, (HMENU)(intptr_t)BUTTON_CLOSE, hInst, NULL);

        ShowWindow(g_hwndSettingsDialog, SW_SHOW);
        return g_hwndSettingsDialog;
    }

    void SettingsDialog::Destroy()
    {
        if (g_hwndSettingsDialog && IsWindow(g_hwndSettingsDialog))
        {
            DestroyWindow(g_hwndSettingsDialog);
        }
        g_hwndSettingsDialog = NULL;
    }

    HWND SettingsDialog::GetHandle()
    {
        return g_hwndSettingsDialog;
    }

    bool SettingsDialog::IsOpen()
    {
        return g_hwndSettingsDialog != NULL && IsWindow(g_hwndSettingsDialog);
    }

    void SettingsDialog::LoadSettingsToUI(HWND hwnd) {}
    void SettingsDialog::SaveSettingsFromUI(HWND hwnd) {}
    void SettingsDialog::EnableControls(HWND hwnd, bool enable) {}
    bool SettingsDialog::ValidatePassword(HWND hwnd) { return true; }
    BOOL CALLBACK SettingsDialog::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) { return FALSE; }

} // namespace RowaPickupSlim
