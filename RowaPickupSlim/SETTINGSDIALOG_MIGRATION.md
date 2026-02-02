# SettingsDialog Migration Summary

## Files Created

### 1. `RowaPickupSlim\SettingsDialog.h`
Header file for the modeless settings dialog with:
- Static methods: `Create()`, `Destroy()`, `GetHandle()`, `IsOpen()`
- Dialog control IDs for all settings fields
- Private helper methods for UI management

### 2. `RowaPickupSlim\SettingsDialog.cpp`
Implementation file containing:
- **Create()** - Creates modeless dialog with all GDI controls
- **LoadSettingsToUI()** - Populates UI from `SharedVariables`
- **SaveSettingsFromUI()** - Writes UI values back to `SharedVariables` and config file
- **EnableControls()** - Enables/disables settings based on password validation
- **DialogProc()** - Message handler for the dialog

### 3. Updated `RowaPickupSlim\main.cpp`
Added:
- `#include "SettingsDialog.h"`
- `WM_RBUTTONDOWN` handler - Right-click opens context menu with "Settings" option
- `WM_DESTROY` cleanup - Calls `SettingsDialog::Destroy()`

## UI Layout (GDI32 Created Controls)

```
+------ RowaPickup Settings Dialog ------+
| IP Address:          [________________] |
| Port:                [________________] |
| Stock Location:      [________________] |
| Pickups Only:        [X]               |
| Scan Output:         [ ]               |
| Read Speed (ms):     [_____]            |
| Output Number:       [____]             |
| Priority:            [Normal___?]       |
| Service Password:    [________]  (masked) |
|                      [Save]  [Cancel]   |
+-----------------------------------+
```

## Control Mapping

| MAUI Control | C++ Control | Control ID | Type |
|--------------|------------|-----------|------|
| Entry (IP) | WC_EDIT | IDC_IPADDRESS | Text |
| Entry (Port) | WC_EDIT | IDC_PORT | Number |
| Entry (Location) | WC_EDIT | IDC_STOCKLOCATION | Text |
| CheckBox (PickupsOnly) | WC_BUTTON (BS_AUTOCHECKBOX) | IDC_PICKUPSONLY | Checkbox |
| CheckBox (ScanOutput) | WC_BUTTON (BS_AUTOCHECKBOX) | IDC_SCANOUTPUT | Checkbox |
| Entry (ReadSpeed) | WC_EDIT | IDC_READSPEED | Number |
| Entry (OutputNumber) | WC_EDIT | IDC_OUTPUTNUMBER | Text |
| Picker (Priority) | WC_COMBOBOX | IDC_PRIORITY | Dropdown |
| Entry (Password) | WC_EDIT (ES_PASSWORD) | IDC_PASSWORD | Password |

## Features Ported from C#

| Feature | C# Implementation | C++ Implementation |
|---------|-------------------|-------------------|
| Settings persistence | XML serialization | INI-style text file |
| Password protection | OnPasswordChanged event | EN_CHANGE notification |
| Validation | Data binding validation | Text field checks |
| Modal behavior | ContentPage | Modeless dialog (resizable) |
| Save action | SaveSettingsCommand | IDC_SAVE button handler |

## Key Differences

1. **Dialog Type**: Modeless (can be left open) vs MAUI ContentPage (single-page)
2. **File Format**: INI-style key=value vs C# XML configuration
3. **Password Check**: Real-time as user types (EN_CHANGE) vs on-submit
4. **Control Enable/Disable**: Direct EnableWindow() calls vs binding-based visibility

## Usage

### Open Settings Dialog
```cpp
// Right-click on main window and select "Settings"
// OR programmatically:
HWND hSettings = SettingsDialog::Create(hMainWindow);
```

### Check if Settings Dialog is Open
```cpp
if (SettingsDialog::IsOpen()) {
    // Settings dialog is currently open
}
```

### Close Settings Dialog
```cpp
SettingsDialog::Destroy();
```

## Config File Location

```
C:\ProgramData\RowaPickupMAUI\RowaPickupMaui.config
```

Example content:
```
ClientIpAddress=127.0.0.1
ClientPort=6050
RobotStockLocation=None
PickupsOnly=true
ScanOutput=false
ReadSpeed=100
OutputNumber=001
PrioPicker=2
PrioPickerText=Normal
```

## Password Protection

- **Behavior**: Settings are disabled until correct password is entered
- **Validation**: Real-time as user types in password field
- **Security Note**: Hardcoded password in source code (consider moving to secure config in production)

## Integration Checklist

- [x] Create `SettingsDialog.h` and `SettingsDialog.cpp`
- [x] Add `#include "SettingsDialog.h"` to main.cpp
- [x] Add right-click menu handler (WM_RBUTTONDOWN)
- [x] Add cleanup in WM_DESTROY
- [ ] Add project files to .vcxproj
- [ ] Compile and test

## Build Requirements

- Windows API: `windows.h`, `commctrl.h`
- Standard library: `fstream`, `cstdio`, `string`
- Link: `comctl32.lib` (already in project)
- Include existing: `SharedVariables.h`

## Future Enhancements

1. Add OK/Apply/Cancel buttons with confirmation dialog
2. Implement reconnect logic after settings save
3. Add validation for IP address and port ranges
4. Support for multiple configuration profiles
5. Encrypt sensitive settings in config file
6. Add tooltips/help text for each setting
7. Theme support (dark mode)
