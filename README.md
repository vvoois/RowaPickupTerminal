# RowaPickupSlim

A lightweight **Windows GDI application** written in modern C++ that replaces a MAUI cross-platform app for warehouse robot picking operations. Communicates with WWKS (Warehouse Workflow Keeping System) servers via TCP/XML.

## Overview

RowaPickupSlim is a terminal-style UI application for managing pharmacy/medical supply warehouse picking operations. It connects to automated storage systems (like Baxter Rowa robots) and displays available stock, allowing operators to:

- **View stock levels** for medical articles
- **Search and filter** articles in real-time
- **Send picking requests** (OutputRequests) to warehouse automation systems
- **Track order status** with color-coded visual feedback
- **Support multi-language** (English & Dutch)
- **Log all operations** for audit trails

## Architecture

### Modular Design

The project uses **header-driven modular architecture** to maintain clean separation of concerns:

```
Headers (Contracts):
??? ArticleManagement.h      ? Stock list operations
??? OutputManagement.h       ? Picking request handling
??? DeviceManagement.h       ? Robot/device status
??? LoggingSystem.h          ? Audit logging
??? UIHelpers.h              ? UI rendering & helpers

Implementation:
??? main.cpp                 ? All module implementations (organized by section)
??? networkclient.cpp        ? TCP/WWKS protocol client
??? SettingsDialog.cpp       ? Settings UI
??? Localization.cpp         ? Language support

Configuration:
??? SharedVariables.cpp      ? App settings from INI file
??? Shared.h                 ? Global state & enums
??? XmlDefinitions.cpp       ? WWKS XML parsing
```

### State Management

Global `AppState` structure holds:
- Article lists (filtered & full)
- Output request tracking
- UI state (selection, scrolling)
- Robot/device status
- Connection state

Protected by mutex for thread-safe access across network receive threads.

### Network Architecture

**NetworkClient** class handles:
- **TCP connection** to WWKS server with auto-reconnection (10 attempts, 5s intervals)
- **WWKS 2.0 protocol** handshake (HelloRequest ? StatusRequest ? StockInfoRequest)
- **KeepAlive management** (automatic response to server pings)
- **State machine** for connection lifecycle (4 states)
- **Async message dispatch** to UI thread via callbacks

## Build Requirements

- **Visual Studio 2026** (v145 Platform Toolset)
- **Windows SDK 10.0.26100+**
- **C++17** or later
- **Minimum Target Platform**: Windows 10 (22H2)

### Compatibility

| Version | Status | Notes |
|---------|--------|-------|
| VS 2026 (v145) | ? **Primary** | Fully supported & tested |
| VS 2022 (v143) | ?? May work | Requires testing & possible adjustments |
| VS 2019 | ? Not supported | Toolset mismatch |
| MinGW/Clang | ? Not supported | Windows-specific APIs (Winsock2, GDI) |

### Dependencies (Included)

- **pugixml** – Lightweight XML parsing
- **Winsock2** – Windows TCP networking
- **Windows GDI** – Native UI rendering

No external package manager needed; all deps are source-included.

## Configuration

Settings are loaded from:
```
C:\ProgramData\RowaPickup\RowaPickupMaui.config
```

INI format example:
```ini
[Connection]
ClientIpAddress=192.168.1.100
ClientPort=102
SourceNumber=1
OutputNumber=OUT_001
RobotStockLocation=LOC_A

[UI]
ScanOutput=1
ReadSpeed=40
Language=EN

[Security]
Password=*****
```

## Features

### UI
- **GDI-based rendering** (no WinForms/XAML dependency)
- **Custom scrollbar** with mouse dragging
- **Real-time search** with debounce
- **Color-coded status** (Blue=Our order, Yellow=Other order, Orange=Picking, Red=Failed, Green=Complete)
- **Tooltip support** for order details
- **Tab navigation** between controls

### Logging
- **Timestamped logs** in `C:\ProgramData\RowaPickup\Protocol\`
- **Auto-cleanup** of logs older than 31 days
- **Network message capture** (incoming/outgoing WWKS XML)
- **State machine tracking**

### Localization
- **English & Dutch** fully supported
- **Easy to extend** with new languages via `.lng` files
- **Real-time language switching** (WM_APP_LANGUAGE_CHANGED)

## Usage

### Build & Run

```bash
# Open RowaPickupSlim.sln in Visual Studio
# Build ? Release or Debug
# Run the .exe
```

### Connect to Server

1. Configure IP/port in settings dialog (right-click ? Settings)
2. Do not edit Stock Location Id or TenantId unless the machine is configured with these options (leave them empty)
3. Click OK ? app connects and loads stock list
4. **Double-click** an article to send picking request
5. **Enter** in search to auto-send if exactly 1 match
6. Right-click menu for additional options

### Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `Tab` / `Shift+Tab` | Navigate between controls |
| `?` / `?` | Select article up/down |
| `Page Up` / `Page Down` | Scroll list |
| `Enter` | Send picking request |
| `Escape` | Clear search |

## WWKS Protocol Support

Implements **WWKS 2.0** message types:

**Outgoing:**
- `HelloRequest` – Initial handshake
- `StatusRequest` – Query robot status
- `StockInfoRequest` – Request inventory
- `OutputRequest` – Send picking order
- `KeepAliveResponse` – Respond to server pings

**Incoming:**
- `HelloResponse`
- `StatusResponse`
- `StockInfoResponse`
- `OutputMessage` / `OutputResponse`
- `TaskInfoResponse`
- `KeepAliveRequest`

## Code Organization

### main.cpp (Core Logic)

Organized by module section markers:

```cpp
// ========== ArticleManagement ==========
find_article_index()
update_article_quantity_in_both_lists()
perform_search_filter()

// ========== OutputManagement ==========
send_output_request_for_article()
update_output_record_from_message()

// ========== DeviceManagement ==========
handle_device_status_updates()

// ========== LoggingSystem ==========
LogMessage()
InitializeLogging()
```

### Threading Model

- **Main UI thread** – WndProc, painting, user input
- **Network receive thread** – Reads TCP socket, dispatches messages
- **Connection polling thread** – Auto-reconnect with backoff
- **Message dispatch threads** – Handle async callbacks

All shared state protected by `g_state.mtx`.

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Can't connect to server | Check IP/port in Settings, ensure server is running |
| Connection times out after 2 min | Server not sending KeepAlive; check firewall |
| Settings won't save | Verify write permissions to `C:\ProgramData\RowaPickup\` |
| Logs not appearing | Check `C:\ProgramData\RowaPickup\Protocol\` folder permissions |
| Crashes on startup | Ensure `RowaPickupMaui.config` exists with valid INI format |

## License

This project is licensed under the **GNU General Public License v3.0** – see [LICENSE](LICENSE) file for details.

## Contributing

Contributions welcome! Please ensure:
- Code follows existing style (modular, minimal comments)
- Headers declare the contract, main.cpp implements
- Thread safety maintained (use mutexes where needed)
- WWKS protocol compliance preserved

## References

- **WWKS 2.0 Specification** – Contact Becton Dickinson Netherlands
- **Baxter Rowa System** – Automated pharmacy storage solution
- **pugixml** – http://pugixml.org/

## Author

Ported from C# MAUI cross-platform app to native Windows C++ (2024).

## Author

**Vincent Voois** – Original developer & C# to C++ port (2024)

- Initial MAUI cross-platform app (C#)
- Windows GDI port & modernization (C++)
- WWKS 2.0 protocol implementation
- Multi-language support & configuration system

---

**Status:** Production-ready | **Language:** Modern C++ (C++17) | **Platform:** Windows 10+
