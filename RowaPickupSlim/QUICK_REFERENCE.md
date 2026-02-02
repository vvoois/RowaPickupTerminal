# RowaPickupSlim C++ Migration Quick Reference

## Project Structure

```
RowaPickupSlim/
??? main.cpp                      # Main application window (WM_CREATE, message loop)
??? networkclient.h/cpp           # TCP client with XML message callback
??? SharedVariables.h/cpp         # Global configuration (loaded from config file)
??? SettingsDialog.h/cpp          # Settings UI dialog (right-click to open)
??? XmlDefinitions.cpp            # XML parsing structs (pugixml)
??? MAUI Project Files (reference only)
    ??? SettingsViewModel.cs       # ? Ported to SettingsDialog.cpp
    ??? SettingsPage.xaml(.cs)     # ? Ported to SettingsDialog.h/.cpp
    ??? SharedVariables.cs         # ? Ported to SharedVariables.h/.cpp
    ??? MainPage.xaml.cs           # ? Ported to main.cpp WndProc
    ??? NetworkClient.cs           # ? Ported to networkclient.h/.cpp
```

## File Porting Status

| MAUI File | C++ Equivalent | Status |
|-----------|----------------|--------|
| MainPage.xaml.cs | main.cpp | ? Complete |
| NetworkClient.cs | networkclient.h/.cpp | ? Complete |
| SharedVariables.cs | SharedVariables.h/.cpp | ? Complete |
| SettingsViewModel.cs | SettingsDialog.cpp | ? Complete |
| SettingsPage.xaml | SettingsDialog.h/.cpp | ? Complete |
| XmlDefinitions.cs | XmlDefinitions.cpp | ? Complete |

## How to Use the Application

### Starting the App
```
RowaPickupSlim.exe
```

### Main Window
- Displays articles with quantity in colored rows
- Up/Down arrows to navigate articles
- Enter to confirm output request
- Right-click for context menu

### Settings Dialog
- Right-click main window ? "Settings"
- Modify network settings, priorities, output location
- Click "Save" to persist changes

### Configuration File
```
C:\ProgramData\RowaPickupMAUI\RowaPickupMaui.config
```

## Key Classes

### Main Application Window
```cpp
// WndProc handles:
WM_CREATE           // Initialize network client
WM_PAINT            // Draw articles + status
WM_LBUTTONDOWN      // Select article
WM_KEYDOWN          // Navigate/confirm
WM_RBUTTONDOWN      // Open context menu
WM_DESTROY          // Cleanup
WM_APP_NETWORK_UPDATE // Repaint on network data
```

### Network Client
```cpp
NetworkClient client;
client.Connect(ip, port);
client.MessageReceived = [](const std::string& type, const std::string& xml) {
    // Handle incoming WWKS XML messages
};
client.SendMessage(xmlString);
```

### Settings
```cpp
// Load settings at startup
SettingsLoader::LoadSettings();

// Access settings globally
SharedVariables::ClientIpAddress
SharedVariables::ClientPort
SharedVariables::SourceNumber
SharedVariables::SelectedPrioItemText
// ... etc
```

### Settings Dialog
```cpp
// Open from main window
SettingsDialog::Create(hMainWindow);

// Check if open
if (SettingsDialog::IsOpen()) { ... }

// Close
SettingsDialog::Destroy();
```

## Building the Project

### Required Dependencies
- **Windows API**: Winsock2, WinINet, commctrl
- **pugixml**: Header-only XML parsing library
- **C++ Standard Library**: threads, mutex, strings

### Visual Studio
1. Add all .cpp files to project
2. Add all .h files to project
3. Link: `Ws2_32.lib`, `comctl32.lib`
4. Build ? Compile

## Message Flow

```
App Start
  ?
WM_CREATE
  ?? Load SharedVariables from config
  ?? Create NetworkClient
  ?? Connect to server
  ?
Network Thread (receive messages)
  ?? Parse WWKS XML
  ?? Update AppState
  ?? Post WM_APP_NETWORK_UPDATE to UI
  ?
UI Thread (WM_PAINT)
  ?? Read AppState
  ?? Draw articles + status
  ?
User Input
  ?? Click/Select ? Update selectedIndex
  ?? Enter ? Send OutputRequest
  ?? Right-click ? Show Settings Menu
  ?
Settings Dialog
  ?? Validate password
  ?? Load current settings
  ?? User edits
  ?? Save ? Update SharedVariables + Config file
  ?? Close
```

## Common Tasks

### Add a New Setting
1. Add to `SharedVariables::` static member
2. Initialize in `SharedVariables.cpp`
3. Add UI control in `SettingsDialog::Create()`
4. Load in `LoadSettingsToUI()`
5. Save in `SaveSettingsFromUI()`
6. Use in `main.cpp` as needed

### Change Color Scheme
Edit in `main.cpp`:
```cpp
static const COLORREF CLR_PURPLE = RGB(128, 0, 128);
static const COLORREF CLR_GREY   = RGB(200, 200, 200);
static const COLORREF CLR_BLUE   = RGB(0, 122, 204);
// ... etc
```

### Add Message Handler
In `WndProc`:
```cpp
case WM_CUSTOM_MESSAGE:
    // Handle message
    return 0;
```

### Post UI Update from Network Thread
```cpp
PostMessage(hMainWindow, WM_APP_NETWORK_UPDATE, 0, 0);
```

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Can't connect to server | Check IP/port in settings, verify server is running |
| Settings won't save | Ensure `C:\ProgramData\RowaPickupMAUI\` directory exists |
| Articles not displaying | Verify StockInfoResponse is being received |
| Colors not showing | Update RGB values in main.cpp |

## References

- **Windows API**: https://docs.microsoft.com/en-us/windows/win32/
- **Winsock2**: https://docs.microsoft.com/en-us/windows/win32/winsock/
- **pugixml**: https://pugixml.org/docs/quickstart.html
- **GDI32**: https://docs.microsoft.com/en-us/windows/win32/gdi/about-gdi

---

**Last Updated**: Migration from MAUI to C++ GDI32  
**Total Files**: 8 source files + headers  
**Build Target**: Windows Desktop (GDI32 + Winsock2)
