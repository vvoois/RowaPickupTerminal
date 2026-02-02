#pragma once
// SettingsDialog.h
// Modeless dialog for application settings (GDI32-based).
// Mirrors SettingsPage.xaml UI layout and SettingsViewModel.cs functionality.

#include <windows.h>
#include <string>

namespace RowaPickupSlim
{
    // Settings Dialog - modeless window for editing application settings
    class SettingsDialog
    {
    public:
        // Create and show the modeless settings dialog
        static HWND Create(HWND hwndParent);

        // Destroy the dialog
        static void Destroy();

        // Get the dialog handle
        static HWND GetHandle();

    // Check if dialog is open
    static bool IsOpen();
    
    // Update UI strings when language changes
    static void UpdateLanguage();

private:
        static HWND s_hwndDialog;
        static BOOL CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        
        // Dialog control IDs
        static constexpr int IDC_IPADDRESS = 1001;
        static constexpr int IDC_PORT = 1002;
        static constexpr int IDC_STOCKLOCATION = 1003;
        static constexpr int IDC_PICKUPSONLY = 1004;
        static constexpr int IDC_SCANOUTPUT = 1005;
        static constexpr int IDC_READSPEED = 1006;
        static constexpr int IDC_OUTPUTNUMBER = 1007;
        static constexpr int IDC_PRIORITY = 1008;
        static constexpr int IDC_PASSWORD = 1009;
        static constexpr int IDC_TENANTID = 1012;  // New TenantId field
        static constexpr int IDC_SAVE = 1010;
        static constexpr int IDC_CANCEL = 1011;

        // Helper functions
        static void LoadSettingsToUI(HWND hwnd);
        static void SaveSettingsFromUI(HWND hwnd);
        static void EnableControls(HWND hwnd, bool enable);
        static bool ValidatePassword(HWND hwnd);
    };

} // namespace RowaPickupSlim
