# SharedVariables Migration Summary

## Files Created

### 1. `RowaPickupSlim\SharedVariables.h`
Header file containing:
- **SharedVariables** class with static properties:
  - `SelectedPrioItem` (int)
  - `SelectedPrioItemText` (string)
  - `SourceNumber` (int)
  - `IsPickupsOnlyChecked` (bool)
  - `ClientIpAddress` (string)
  - `ClientPort` (int)
  - `RobotStockLocation` (string)
  - `ScanOutput` (bool)
  - `ReadSpeed` (string)
  - `OutputNumber` (string)

- **TemporaryArticle** struct for article data

- **SettingsLoader** class with static `LoadSettings()` method

### 2. `RowaPickupSlim\SharedVariables.cpp`
Implementation file containing:
- Static member initialization with C# defaults
- `GetConfigFilePath()` - finds `CommonApplicationData\RowaPickupMAUI\RowaPickupMaui.config`
- `LoadSettings()` - reads INI-style config file and populates SharedVariables
- `ParseBool()` - converts string to boolean
- Utility functions for string trimming and case conversion

## Configuration File Format

The config file should be located at:
```
C:\ProgramData\RowaPickupMAUI\RowaPickupMaui.config
```

Example format:
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

## Key Differences from C#

| Feature | C# | C++ |
|---------|----|----|
| Async | `async Task LoadSettingsAsync()` | Synchronous `bool LoadSettings()` |
| Platform detection | MAUI `DeviceInfo` | Windows-only with `SHGetFolderPath` |
| Random number | `new Random()` | `std::mt19937` with `std::uniform_int_distribution` |
| Boolean parsing | `Convert.ToBoolean()` | Custom `ParseBool()` |
| Debug output | `Debug.WriteLine()` | `OutputDebugStringA()` |

## Usage in main.cpp

In `WM_CREATE` handler:
```cpp
// Load settings from config file before connecting
SettingsLoader::LoadSettings();

// Now use SharedVariables for connection
g_client->Connect(SharedVariables::ClientIpAddress, SharedVariables::ClientPort);
```

## Integration Steps

1. Add both files to your project:
   - `RowaPickupSlim\SharedVariables.h`
   - `RowaPickupSlim\SharedVariables.cpp`

2. Include header in `main.cpp` (already done):
   ```cpp
   #include "SharedVariables.h"
   using namespace RowaPickupSlim;
   ```

3. Call `SettingsLoader::LoadSettings()` at application startup

4. Create config file in `C:\ProgramData\RowaPickupMAUI\RowaPickupMaui.config`

5. Reference `SharedVariables::` properties throughout the application

## Build Requirements

- Windows API (`windows.h`, `shlobj.h`)
- Standard library (`string`, `vector`, `fstream`, `random`)
- No external dependencies beyond what main.cpp already requires
